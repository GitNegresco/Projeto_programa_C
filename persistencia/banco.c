
#include "banco.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int resultado_tuplas_ok(PGconn *conexao, PGresult *resultado,
                               const char *operacao)
{
    if (PQresultStatus(resultado) == PGRES_TUPLES_OK) {
        return 1;
    }

    fprintf(stderr, "Erro ao %s: %s", operacao, PQerrorMessage(conexao));
    return 0;
}

static int executar_comando(PGconn *conexao, const char *sql)
{
    PGresult *resultado = PQexec(conexao, sql);
    int ok;

    if (resultado == NULL) {
        fprintf(stderr, "Erro no banco: %s", PQerrorMessage(conexao));
        return 0;
    }
    ok = PQresultStatus(resultado) == PGRES_COMMAND_OK;
    if (!ok) {
        fprintf(stderr, "Erro no banco: %s", PQerrorMessage(conexao));
    }
    PQclear(resultado);
    return ok;
}

int banco_conectar(PGconn **conexao)
{
    const char *url = getenv("DATABASE_URL");
    const char *parametros = "dbname=Campeonato_Esports";

    if (conexao == NULL) {
        fprintf(stderr, "Ponteiro de conexao invalido.\n");
        return 0;
    }
    if (url != NULL && url[0] != '\0') {
        parametros = url;
    } else if (getenv("PGPASSWORD") == NULL ||
               getenv("PGPASSWORD")[0] == '\0') {
        fprintf(stderr,
                "Configure PGPASSWORD no ambiente antes de conectar.\n");
        return 0;
    }

    *conexao = PQconnectdb(parametros);
    if (*conexao == NULL || PQstatus(*conexao) != CONNECTION_OK) {
        fprintf(stderr, "Nao foi possivel conectar ao PostgreSQL.\n");
        if (*conexao != NULL) {
            fprintf(stderr, "%s", PQerrorMessage(*conexao));
            PQfinish(*conexao);
            *conexao = NULL;
        }
        return 0;
    }

    return 1;
}

void banco_desconectar(PGconn *conexao)
{
    if (conexao != NULL) {
        PQfinish(conexao);
    }
}

int banco_carregar_equipes(PGconn *conexao, Equipe equipes[],
                           int capacidade, int *quantidade)
{
    const char *sql =
        "SELECT e.id_equipe, COALESCE(e.nome_equipe, ''), COUNT(j.cpf) "
        "FROM equipes e LEFT JOIN jogadores j "
        "ON j.fk_equipes_id_equipe = e.id_equipe "
        "GROUP BY e.id_equipe, e.nome_equipe ORDER BY e.id_equipe";
    PGresult *resultado = PQexec(conexao, sql);
    int linhas;
    int i;

    if (resultado == NULL ||
        !resultado_tuplas_ok(conexao, resultado, "carregar equipes")) {
        if (resultado != NULL) PQclear(resultado);
        return 0;
    }
    linhas = PQntuples(resultado);
    if (linhas > capacidade) {
        fprintf(stderr, "O banco possui mais equipes que o limite (%d).\n",
                capacidade);
        PQclear(resultado);
        return 0;
    }

    for (i = 0; i < linhas; i++) {
        const char *nome = PQgetvalue(resultado, i, 1);
        if (strlen(nome) >= sizeof(equipes[i].nome_equipe)) {
            fprintf(stderr, "O nome de uma equipe excede o limite suportado.\n");
            PQclear(resultado);
            return 0;
        }
        equipes[i].id_equipe = atoi(PQgetvalue(resultado, i, 0));
        strcpy(equipes[i].nome_equipe, nome);
        equipes[i].num_jogadores = atoi(PQgetvalue(resultado, i, 2));
    }

    *quantidade = linhas;
    PQclear(resultado);
    return 1;
}

int banco_carregar_jogadores(PGconn *conexao, Jogador jogadores[],
                             int capacidade, int *quantidade)
{
    const char *sql =
        "SELECT cpf, COALESCE(apelido, ''), "
        "COALESCE(fk_equipes_id_equipe, -1) "
        "FROM jogadores ORDER BY cpf";
    PGresult *resultado = PQexec(conexao, sql);
    int linhas;
    int i;

    if (resultado == NULL ||
        !resultado_tuplas_ok(conexao, resultado, "carregar jogadores")) {
        if (resultado != NULL) PQclear(resultado);
        return 0;
    }
    linhas = PQntuples(resultado);
    if (linhas > capacidade) {
        fprintf(stderr, "O banco possui mais jogadores que o limite (%d).\n",
                capacidade);
        PQclear(resultado);
        return 0;
    }

    for (i = 0; i < linhas; i++) {
        const char *apelido = PQgetvalue(resultado, i, 1);
        if (strlen(apelido) >= sizeof(jogadores[i].nickname)) {
            fprintf(stderr, "O apelido de um jogador excede o limite suportado.\n");
            PQclear(resultado);
            return 0;
        }
        jogadores[i].id_jogador = atoi(PQgetvalue(resultado, i, 0));
        strcpy(jogadores[i].nickname, apelido);
        jogadores[i].id_equipe = atoi(PQgetvalue(resultado, i, 2));
    }

    *quantidade = linhas;
    PQclear(resultado);
    return 1;
}

static int converter_double(const char *texto, double *valor)
{
    char *fim;

    errno = 0;
    *valor = strtod(texto, &fim);
    return errno == 0 && fim != texto && *fim == '\0';
}

int banco_carregar_partidas(PGconn *conexao, Partida partidas[],
                            int capacidade, int *quantidade)
{
    const char *sql =
        "SELECT p.id_partida, COALESCE(p.num_rodada, 0), "
        "r.fk_equipes_id_equipe, r.resultado_partida "
        "FROM partidas p LEFT JOIN resultados r "
        "ON r.fk_partidas_id_partida = p.id_partida "
        "ORDER BY p.id_partida, r.id_resultado";
    PGresult *resultado = PQexec(conexao, sql);
    int linhas;
    int i;
    int qtd = 0;
    int partida_atual = -1;
    int partida_iniciada = 0;
    int resultados_partida = 0;
    Partida pendente;

    if (resultado == NULL ||
        !resultado_tuplas_ok(conexao, resultado, "carregar partidas")) {
        if (resultado != NULL) PQclear(resultado);
        return 0;
    }
    linhas = PQntuples(resultado);

    for (i = 0; i < linhas; i++) {
        int id = atoi(PQgetvalue(resultado, i, 0));
        int equipe;
        double placar;

        if (!partida_iniciada || id != partida_atual) {
            if (resultados_partida != 0 && resultados_partida != 2) {
                fprintf(stderr,
                        "A partida %d nao possui exatamente dois resultados.\n",
                        partida_atual);
                PQclear(resultado);
                return 0;
            }
            if (resultados_partida == 2) {
                if (qtd == capacidade) {
                    fprintf(stderr,
                            "O banco possui mais partidas que o limite (%d).\n",
                            capacidade);
                    PQclear(resultado);
                    return 0;
                }
                partidas[qtd++] = pendente;
            }
            memset(&pendente, 0, sizeof(pendente));
            pendente.id_partida = id;
            pendente.rodada = atoi(PQgetvalue(resultado, i, 1));
            partida_atual = id;
            partida_iniciada = 1;
            resultados_partida = 0;
        }

        if (PQgetisnull(resultado, i, 2) ||
            PQgetisnull(resultado, i, 3) ||
            !converter_double(PQgetvalue(resultado, i, 3), &placar)) {
            fprintf(stderr,
                    "A partida %d possui um resultado incompleto ou invalido.\n",
                    id);
            PQclear(resultado);
            return 0;
        }
        equipe = atoi(PQgetvalue(resultado, i, 2));
        if (resultados_partida == 0) {
            pendente.id_equipeA = equipe;
            pendente.placarA = placar;
        } else if (resultados_partida == 1) {
            if (equipe == pendente.id_equipeA) {
                fprintf(stderr,
                        "A partida %d possui resultados duplicados para a mesma equipe.\n",
                        id);
                PQclear(resultado);
                return 0;
            }
            pendente.id_equipeB = equipe;
            pendente.placarB = placar;
        } else {
            fprintf(stderr, "A partida %d possui mais de dois resultados.\n", id);
            PQclear(resultado);
            return 0;
        }
        resultados_partida++;
    }

    if (resultados_partida != 0 && resultados_partida != 2) {
        fprintf(stderr,
                "A partida %d nao possui exatamente dois resultados.\n",
                partida_atual);
        PQclear(resultado);
        return 0;
    }
    if (resultados_partida == 2) {
        if (qtd == capacidade) {
            fprintf(stderr, "O banco possui mais partidas que o limite (%d).\n",
                    capacidade);
            PQclear(resultado);
            return 0;
        }
        partidas[qtd++] = pendente;
    }

    *quantidade = qtd;
    PQclear(resultado);
    return 1;
}

int banco_salvar_equipe(PGconn *conexao, const char nome[], int *id_equipe)
{
    PGresult *resultado;
    char id_texto[32];
    const char *parametros[2];

    if (!executar_comando(conexao, "BEGIN") ||
        !executar_comando(conexao, "LOCK TABLE equipes IN EXCLUSIVE MODE")) {
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }

    resultado = PQexec(conexao,
                       "SELECT COALESCE(MAX(id_equipe), 0) + 1 FROM equipes");
    if (resultado == NULL ||
        !resultado_tuplas_ok(conexao, resultado, "gerar ID da equipe")) {
        if (resultado != NULL) PQclear(resultado);
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    snprintf(id_texto, sizeof(id_texto), "%s", PQgetvalue(resultado, 0, 0));
    PQclear(resultado);

    parametros[0] = id_texto;
    parametros[1] = nome;
    resultado = PQexecParams(
        conexao,
        "INSERT INTO equipes (id_equipe, nome_equipe) VALUES ($1, $2)",
        2, NULL, parametros, NULL, NULL, 0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Erro ao cadastrar equipe: %s", PQerrorMessage(conexao));
        if (resultado != NULL) PQclear(resultado);
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    PQclear(resultado);

    if (!executar_comando(conexao, "COMMIT")) {
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    *id_equipe = atoi(id_texto);
    return 1;
}

int banco_salvar_partida(PGconn *conexao, Partida *partida)
{
    PGresult *resultado;
    char id_partida_texto[32];
    char id_resultado_a_texto[32];
    char id_resultado_b_texto[32];
    char rodada_texto[32];
    char campeonato_texto[32];
    char jogo_texto[32];
    char equipe_a_texto[32];
    char equipe_b_texto[32];
    char placar_a_texto[64];
    char placar_b_texto[64];
    const char *parametros[6];

    if (!executar_comando(conexao, "BEGIN") ||
        !executar_comando(conexao,
                          "LOCK TABLE partidas, resultados IN EXCLUSIVE MODE")) {
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }

    resultado = PQexec(
        conexao,
        "SELECT COALESCE(MAX(id_partida), 0) + 1, "
        "(SELECT COALESCE(MAX(id_resultado), 0) + 1 FROM resultados) "
        "FROM partidas");
    if (resultado == NULL ||
        !resultado_tuplas_ok(conexao, resultado, "gerar IDs da partida")) {
        if (resultado != NULL) PQclear(resultado);
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    snprintf(id_partida_texto, sizeof(id_partida_texto), "%s",
             PQgetvalue(resultado, 0, 0));
    snprintf(id_resultado_a_texto, sizeof(id_resultado_a_texto), "%s",
             PQgetvalue(resultado, 0, 1));
    PQclear(resultado);
    {
        long id_resultado_b = strtol(id_resultado_a_texto, NULL, 10) + 1;
        snprintf(id_resultado_b_texto, sizeof(id_resultado_b_texto), "%ld",
                 id_resultado_b);
    }

    snprintf(rodada_texto, sizeof(rodada_texto), "%d", partida->rodada);
    snprintf(campeonato_texto, sizeof(campeonato_texto), "%d",
             partida->id_campeonato);
    snprintf(jogo_texto, sizeof(jogo_texto), "%d", partida->id_jogo);
    parametros[0] = id_partida_texto;
    parametros[1] = rodada_texto;
    parametros[2] = campeonato_texto;
    parametros[3] = jogo_texto;
    resultado = PQexecParams(
        conexao,
        "INSERT INTO partidas (id_partida, num_rodada, "
        "fk_campeonato_id_campeonato, fk_jogos_id_jogo) "
        "VALUES ($1, $2, $3, $4)",
        4, NULL, parametros, NULL, NULL, 0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Erro ao cadastrar partida: %s", PQerrorMessage(conexao));
        if (resultado != NULL) PQclear(resultado);
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    PQclear(resultado);

    snprintf(equipe_a_texto, sizeof(equipe_a_texto), "%d",
             partida->id_equipeA);
    snprintf(equipe_b_texto, sizeof(equipe_b_texto), "%d",
             partida->id_equipeB);
    snprintf(placar_a_texto, sizeof(placar_a_texto), "%.17g",
             partida->placarA);
    snprintf(placar_b_texto, sizeof(placar_b_texto), "%.17g",
             partida->placarB);
    parametros[0] = id_resultado_a_texto;
    parametros[1] = placar_a_texto;
    parametros[2] = equipe_a_texto;
    parametros[3] = id_partida_texto;
    resultado = PQexecParams(
        conexao,
        "INSERT INTO resultados (id_resultado, resultado_partida, "
        "fk_equipes_id_equipe, fk_partidas_id_partida) "
        "VALUES ($1, $2, $3, $4)",
        4, NULL, parametros, NULL, NULL, 0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Erro ao gravar resultado da equipe A: %s",
                PQerrorMessage(conexao));
        if (resultado != NULL) PQclear(resultado);
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    PQclear(resultado);

    parametros[0] = id_resultado_b_texto;
    parametros[1] = placar_b_texto;
    parametros[2] = equipe_b_texto;
    resultado = PQexecParams(
        conexao,
        "INSERT INTO resultados (id_resultado, resultado_partida, "
        "fk_equipes_id_equipe, fk_partidas_id_partida) "
        "VALUES ($1, $2, $3, $4)",
        4, NULL, parametros, NULL, NULL, 0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Erro ao gravar resultado da equipe B: %s",
                PQerrorMessage(conexao));
        if (resultado != NULL) PQclear(resultado);
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    PQclear(resultado);

    if (!executar_comando(conexao, "COMMIT")) {
        executar_comando(conexao, "ROLLBACK");
        return 0;
    }
    partida->id_partida = atoi(id_partida_texto);
    return 1;
}

static int listar_tabela(PGconn *conexao, const char *sql,
                         const char *titulo, int coluna_nome)
{
    PGresult *resultado = PQexec(conexao, sql);
    int linhas;
    int i;

    if (resultado == NULL ||
        !resultado_tuplas_ok(conexao, resultado, titulo)) {
        if (resultado != NULL) PQclear(resultado);
        return -1;
    }
    linhas = PQntuples(resultado);
    printf("\n--- %s ---\n", titulo);
    for (i = 0; i < linhas; i++) {
        printf("ID %s | %s\n", PQgetvalue(resultado, i, 0),
               PQgetvalue(resultado, i, coluna_nome));
    }
    if (linhas == 0) {
        printf("Nenhum registro cadastrado.\n");
    }
    PQclear(resultado);
    return linhas;
}

int banco_listar_campeonatos(PGconn *conexao)
{
    return listar_tabela(
        conexao,
        "SELECT id_campeonato, COALESCE(nome_campeonato, '') "
        "FROM campeonato ORDER BY id_campeonato",
        "CAMPEONATOS", 1);
}

int banco_listar_jogos(PGconn *conexao)
{
    return listar_tabela(
        conexao,
        "SELECT id_jogo, COALESCE(nome, '') FROM jogos ORDER BY id_jogo",
        "JOGOS", 1);
}
