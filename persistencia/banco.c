#include "banco.h"

#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ADAPTAR (se necessario): nomes dos campos usados das structs do entidade.h
 *   Equipe : id_equipe, nome_equipe[], num_jogadores
 *   Jogador: id_jogador, nickname[], id_equipe
 *   Partida: id_partida, rodada, id_campeonato, id_jogo,
 *            id_equipeA, id_equipeB, placarA, placarB
 * Se os seus campos tiverem outro nome, troque so nas funcoes
 * carregar_equipes, carregar_jogadores e carregar_partidas. */

static PGconn *conn = NULL;

/* ------------------------------------------------------------------ */
/* Funcoes auxiliares                                                  */
/* ------------------------------------------------------------------ */

/* Executa um comando com parametros e confere o status.
 * Devolve o resultado (quem chamou deve fazer PQclear) ou NULL se falhar. */
static PGresult *executar(const char *sql, int quantidade,
                          const char *const parametros[],
                          ExecStatusType esperado, const char *operacao)
{
    PGresult *resultado = PQexecParams(conn, sql, quantidade, NULL,
                                       parametros, NULL, NULL, 0);

    if (resultado == NULL) {
        fprintf(stderr, "%s: %s", operacao, PQerrorMessage(conn));
        return NULL;
    }

    if (PQresultStatus(resultado) != esperado) {
        fprintf(stderr, "%s: %s", operacao, PQresultErrorMessage(resultado));
        PQclear(resultado);
        return NULL;
    }

    return resultado;
}

/* BEGIN, COMMIT e ROLLBACK. Devolve 1 se deu certo, 0 se falhou. */
static int comando_simples(const char *sql)
{
    PGresult *resultado = executar(sql, 0, NULL, PGRES_COMMAND_OK, sql);

    if (resultado == NULL) {
        return 0;
    }

    PQclear(resultado);
    return 1;
}

/* INSERT ... RETURNING id. Devolve o id (> 0) ou BANCO_ERRO_BANCO. */
static int inserir_retornando_id(const char *sql, int quantidade,
                                 const char *const parametros[],
                                 const char *operacao)
{
    PGresult *resultado = executar(sql, quantidade, parametros,
                                   PGRES_TUPLES_OK, operacao);
    int id;

    if (resultado == NULL) {
        return BANCO_ERRO_BANCO;
    }

    if (PQntuples(resultado) != 1) {
        fprintf(stderr, "%s: o banco nao devolveu o id.\n", operacao);
        PQclear(resultado);
        return BANCO_ERRO_BANCO;
    }

    id = atoi(PQgetvalue(resultado, 0, 0));
    PQclear(resultado);

    return id > 0 ? id : BANCO_ERRO_BANCO;
}

/* INSERT, UPDATE ou DELETE que deve afetar exatamente 1 linha.
 * Devolve BANCO_OK, BANCO_ERRO_BANCO ou BANCO_ERRO_NAO_ENCONTRADO. */
static int alterar_uma_linha(const char *sql, int quantidade,
                             const char *const parametros[],
                             const char *operacao)
{
    PGresult *resultado = executar(sql, quantidade, parametros,
                                   PGRES_COMMAND_OK, operacao);
    int linhas;

    if (resultado == NULL) {
        return BANCO_ERRO_BANCO;
    }

    linhas = atoi(PQcmdTuples(resultado));
    PQclear(resultado);

    return linhas == 1 ? BANCO_OK : BANCO_ERRO_NAO_ENCONTRADO;
}

/* ------------------------------------------------------------------ */
/* Conexao                                                             */
/* ------------------------------------------------------------------ */

int banco_conectar(void)
{
    const char *url = getenv("DATABASE_URL");

    if (conn != NULL) {
        return BANCO_OK;
    }

    /* String vazia: a libpq usa PGHOST, PGPORT, PGUSER, PGPASSWORD, PGDATABASE */
    conn = PQconnectdb(url != NULL && url[0] != '\0' ? url : "");

    if (conn == NULL) {
        fprintf(stderr, "Nao foi possivel iniciar a conexao PostgreSQL.\n");
        return BANCO_ERRO_BANCO;
    }

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Falha na conexao PostgreSQL: %s", PQerrorMessage(conn));
        PQfinish(conn);
        conn = NULL;
        return BANCO_ERRO_BANCO;
    }

    return BANCO_OK;
}

void banco_desconectar(void)
{
    if (conn != NULL) {
        PQfinish(conn);
        conn = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Insercoes                                                           */
/* ------------------------------------------------------------------ */

int banco_inserir_jogador(const char *nick)
{
    const char *parametros[1];

    if (conn == NULL || nick == NULL || nick[0] == '\0') {
        return BANCO_ERRO_ARGUMENTO;
    }

    parametros[0] = nick;
    return inserir_retornando_id(
        "INSERT INTO jogadores (nickname) VALUES ($1) RETURNING id_jogador",
        1, parametros, "Erro ao inserir jogador");
}

int banco_inserir_equipe(const char *nome)
{
    const char *parametros[1];

    if (conn == NULL || nome == NULL || nome[0] == '\0') {
        return BANCO_ERRO_ARGUMENTO;
    }

    parametros[0] = nome;
    return inserir_retornando_id(
        "INSERT INTO equipes (nome_equipe) VALUES ($1) RETURNING id_equipe",
        1, parametros, "Erro ao inserir equipe");
}

/* Grava a partida e os dois resultados (lado A primeiro) numa transacao:
 * ou grava tudo, ou nao grava nada. Campeonato e jogo ficam no padrao (id 1). */
int banco_inserir_partida(int idA, int idB, int placarA, int placarB, int rodada)
{
    char texto_rodada[32];
    char texto_id_partida[32];
    char texto_idA[32];
    char texto_idB[32];
    char texto_placarA[32];
    char texto_placarB[32];
    const char *parametros_partida[1];
    const char *parametros_resultado[3];
    int id_partida;

    if (conn == NULL || idA <= 0 || idB <= 0 || idA == idB ||
        placarA < 0 || placarB < 0 || rodada <= 0) {
        return BANCO_ERRO_ARGUMENTO;
    }

    snprintf(texto_rodada, sizeof(texto_rodada), "%d", rodada);
    snprintf(texto_idA, sizeof(texto_idA), "%d", idA);
    snprintf(texto_idB, sizeof(texto_idB), "%d", idB);
    snprintf(texto_placarA, sizeof(texto_placarA), "%d", placarA);
    snprintf(texto_placarB, sizeof(texto_placarB), "%d", placarB);

    if (!comando_simples("BEGIN")) {
        return BANCO_ERRO_BANCO;
    }

    parametros_partida[0] = texto_rodada;
    id_partida = inserir_retornando_id(
        "INSERT INTO partidas (num_rodada) VALUES ($1) RETURNING id_partida",
        1, parametros_partida, "Erro ao inserir partida");
    if (id_partida < 0) {
        comando_simples("ROLLBACK");
        return BANCO_ERRO_BANCO;
    }

    snprintf(texto_id_partida, sizeof(texto_id_partida), "%d", id_partida);

    parametros_resultado[0] = texto_id_partida;
    parametros_resultado[1] = texto_idA;
    parametros_resultado[2] = texto_placarA;
    if (alterar_uma_linha(
            "INSERT INTO resultados (id_partida, id_equipe, resultado_partida) "
            "VALUES ($1, $2, $3)",
            3, parametros_resultado,
            "Erro ao inserir o resultado da equipe A") != BANCO_OK) {
        comando_simples("ROLLBACK");
        return BANCO_ERRO_BANCO;
    }

    parametros_resultado[1] = texto_idB;
    parametros_resultado[2] = texto_placarB;
    if (alterar_uma_linha(
            "INSERT INTO resultados (id_partida, id_equipe, resultado_partida) "
            "VALUES ($1, $2, $3)",
            3, parametros_resultado,
            "Erro ao inserir o resultado da equipe B") != BANCO_OK) {
        comando_simples("ROLLBACK");
        return BANCO_ERRO_BANCO;
    }

    if (!comando_simples("COMMIT")) {
        comando_simples("ROLLBACK");
        return BANCO_ERRO_BANCO;
    }

    return id_partida;
}

/* ------------------------------------------------------------------ */
/* Remocao e atualizacao                                               */
/* ------------------------------------------------------------------ */

int banco_remover_jogador(int id)
{
    char texto_id[32];
    const char *parametros[1];

    if (conn == NULL || id <= 0) {
        return BANCO_ERRO_ARGUMENTO;
    }

    snprintf(texto_id, sizeof(texto_id), "%d", id);
    parametros[0] = texto_id;
    return alterar_uma_linha("DELETE FROM jogadores WHERE id_jogador = $1",
                             1, parametros, "Erro ao remover jogador");
}

/* O banco recusa remover equipe que ja jogou partidas (devolve BANCO_ERRO_BANCO).
 * Os jogadores da equipe removida ficam sem equipe. */
int banco_remover_equipe(int id)
{
    char texto_id[32];
    const char *parametros[1];

    if (conn == NULL || id <= 0) {
        return BANCO_ERRO_ARGUMENTO;
    }

    snprintf(texto_id, sizeof(texto_id), "%d", id);
    parametros[0] = texto_id;
    return alterar_uma_linha("DELETE FROM equipes WHERE id_equipe = $1",
                             1, parametros, "Erro ao remover equipe");
}

int banco_atualizar_equipe_jogador(int id_jogador, int id_equipe)
{
    char texto_jogador[32];
    char texto_equipe[32];
    const char *parametros[2];

    if (conn == NULL || id_jogador <= 0) {
        return BANCO_ERRO_ARGUMENTO;
    }

    snprintf(texto_jogador, sizeof(texto_jogador), "%d", id_jogador);
    snprintf(texto_equipe, sizeof(texto_equipe), "%d", id_equipe);

    /* NULL no parametro vira NULL no banco: jogador sem equipe */
    parametros[0] = id_equipe > BANCO_SEM_EQUIPE ? texto_equipe : NULL;
    parametros[1] = texto_jogador;
    return alterar_uma_linha(
        "UPDATE jogadores SET id_equipe = $1 WHERE id_jogador = $2",
        2, parametros, "Erro ao atualizar a equipe do jogador");
}

/* ------------------------------------------------------------------ */
/* Carga inicial                                                       */
/* ------------------------------------------------------------------ */

/* num_jogadores nao existe no banco: e' calculado contando os jogadores */
static int carregar_equipes(Equipe equipes[], int capacidade, int *quantidade)
{
    PGresult *resultado = executar(
        "SELECT e.id_equipe, e.nome_equipe, COUNT(j.id_jogador) "
        "FROM equipes e LEFT JOIN jogadores j ON j.id_equipe = e.id_equipe "
        "GROUP BY e.id_equipe, e.nome_equipe ORDER BY e.id_equipe",
        0, NULL, PGRES_TUPLES_OK, "Erro ao carregar equipes");
    int linhas;
    int i;

    if (resultado == NULL) {
        return BANCO_ERRO_BANCO;
    }

    linhas = PQntuples(resultado);
    if (linhas > capacidade) {
        fprintf(stderr, "O banco tem mais equipes que o limite de %d.\n", capacidade);
        PQclear(resultado);
        return BANCO_ERRO_ARGUMENTO;
    }

    for (i = 0; i < linhas; i++) {
        equipes[i].id_equipe = atoi(PQgetvalue(resultado, i, 0));
        snprintf(equipes[i].nome_equipe, sizeof(equipes[i].nome_equipe),
                 "%s", PQgetvalue(resultado, i, 1));
        equipes[i].num_jogadores = atoi(PQgetvalue(resultado, i, 2));
    }

    *quantidade = linhas;
    PQclear(resultado);
    return BANCO_OK;
}

static int carregar_jogadores(Jogador jogadores[], int capacidade, int *quantidade)
{
<<<<<<< HEAD
    PGresult *resultado = executar(
        "SELECT id_jogador, nickname, id_equipe FROM jogadores ORDER BY id_jogador",
        0, NULL, PGRES_TUPLES_OK, "Erro ao carregar jogadores");
=======
    const char *sql =
        "SELECT cpf, COALESCE(apelido, ''), COALESCE(nome_completo, ''), "
        "COALESCE(fk_equipes_id_equipe, -1) "
        "FROM jogadores ORDER BY cpf";
    PGresult *resultado = PQexec(conexao, sql);
>>>>>>> c1570ca (arrumei uns baguio)
    int linhas;
    int i;

    if (resultado == NULL) {
        return BANCO_ERRO_BANCO;
    }

    linhas = PQntuples(resultado);
    if (linhas > capacidade) {
        fprintf(stderr, "O banco tem mais jogadores que o limite de %d.\n", capacidade);
        PQclear(resultado);
        return BANCO_ERRO_ARGUMENTO;
    }

    for (i = 0; i < linhas; i++) {
<<<<<<< HEAD
        jogadores[i].id_jogador = atoi(PQgetvalue(resultado, i, 0));
        snprintf(jogadores[i].nickname, sizeof(jogadores[i].nickname),
                 "%s", PQgetvalue(resultado, i, 1));
        /* id_equipe vazio no banco (NULL) vira BANCO_SEM_EQUIPE */
        jogadores[i].id_equipe = PQgetisnull(resultado, i, 2)
                                     ? BANCO_SEM_EQUIPE
                                     : atoi(PQgetvalue(resultado, i, 2));
=======
        const char *apelido = PQgetvalue(resultado, i, 1);
        const char *nome_completo = PQgetvalue(resultado, i, 2);
        if (strlen(apelido) >= sizeof(jogadores[i].nickname)) {
            fprintf(stderr, "O apelido de um jogador excede o limite suportado.\n");
            PQclear(resultado);
            return 0;
        }
        if (strlen(nome_completo) >= sizeof(jogadores[i].nome_completo)) {
            fprintf(stderr, "O nome de um jogador excede o limite suportado.\n");
            PQclear(resultado);
            return 0;
        }
        jogadores[i].id_jogador = atoi(PQgetvalue(resultado, i, 0));
        strcpy(jogadores[i].nickname, apelido);
        strcpy(jogadores[i].nome_completo, nome_completo);
        jogadores[i].id_equipe = atoi(PQgetvalue(resultado, i, 3));
>>>>>>> c1570ca (arrumei uns baguio)
    }

    *quantidade = linhas;
    PQclear(resultado);
    return BANCO_OK;
}

/* Cada partida junta seus dois resultados: o de menor id_resultado e' o lado A
 * (o primeiro gravado por banco_inserir_partida) e o de maior e' o lado B. */
static int carregar_partidas(Partida partidas[], int capacidade, int *quantidade)
{
    PGresult *resultado = executar(
        "SELECT p.id_partida, p.num_rodada, p.id_campeonato, p.id_jogo, "
        "a.id_equipe, a.resultado_partida, b.id_equipe, b.resultado_partida "
        "FROM partidas p "
        "JOIN resultados a ON a.id_partida = p.id_partida "
        "JOIN resultados b ON b.id_partida = p.id_partida "
        "                 AND b.id_resultado > a.id_resultado "
        "ORDER BY p.id_partida",
        0, NULL, PGRES_TUPLES_OK, "Erro ao carregar partidas");
    int linhas;
    int i;

    if (resultado == NULL) {
        return BANCO_ERRO_BANCO;
    }

    linhas = PQntuples(resultado);
    if (linhas > capacidade) {
        fprintf(stderr, "O banco tem mais partidas que o limite de %d.\n", capacidade);
        PQclear(resultado);
        return BANCO_ERRO_ARGUMENTO;
    }

<<<<<<< HEAD
=======
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

int banco_salvar_jogador(PGconn *conexao, int cpf,
                         const char nome_completo[], const char apelido[],
                         const char conta_steam[], const char telefone[],
                         int id_equipe)
{
    PGresult *resultado;
    char cpf_texto[32];
    char equipe_texto[32];
    const char *parametros[6];

    snprintf(cpf_texto, sizeof(cpf_texto), "%d", cpf);
    if (id_equipe != SEM_EQUIPE) {
        snprintf(equipe_texto, sizeof(equipe_texto), "%d", id_equipe);
    }

    parametros[0] = cpf_texto;
    parametros[1] = nome_completo;
    parametros[2] = apelido;
    parametros[3] = conta_steam[0] == '\0' ? NULL : conta_steam;
    parametros[4] = telefone[0] == '\0' ? NULL : telefone;
    parametros[5] = id_equipe == SEM_EQUIPE ? NULL : equipe_texto;
    resultado = PQexecParams(
        conexao,
        "INSERT INTO jogadores "
        "(cpf, nome_completo, apelido, conta_steam, telefone, "
        "fk_equipes_id_equipe) VALUES ($1, $2, $3, $4, $5, $6)",
        6, NULL, parametros, NULL, NULL, 0);
    if (resultado == NULL ||
        PQresultStatus(resultado) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Erro ao cadastrar jogador: %s",
                PQerrorMessage(conexao));
        if (resultado != NULL) PQclear(resultado);
        return 0;
    }
    PQclear(resultado);
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
>>>>>>> c1570ca (arrumei uns baguio)
    for (i = 0; i < linhas; i++) {
        partidas[i].id_partida    = atoi(PQgetvalue(resultado, i, 0));
        partidas[i].rodada        = atoi(PQgetvalue(resultado, i, 1));
        partidas[i].id_campeonato = atoi(PQgetvalue(resultado, i, 2));
        partidas[i].id_jogo       = atoi(PQgetvalue(resultado, i, 3));
        partidas[i].id_equipeA    = atoi(PQgetvalue(resultado, i, 4));
        partidas[i].placarA       = atoi(PQgetvalue(resultado, i, 5));
        partidas[i].id_equipeB    = atoi(PQgetvalue(resultado, i, 6));
        partidas[i].placarB       = atoi(PQgetvalue(resultado, i, 7));

        if (i > 0 && partidas[i].id_partida == partidas[i - 1].id_partida) {
            fprintf(stderr, "A partida %d tem mais de dois resultados.\n",
                    partidas[i].id_partida);
            PQclear(resultado);
            return BANCO_ERRO_BANCO;
        }
    }

    *quantidade = linhas;
    PQclear(resultado);
    return BANCO_OK;
}

int banco_carregar_tudo(Equipe equipes[], int cap_equipes, int *qtd_equipes,
                        Jogador jogadores[], int cap_jogadores, int *qtd_jogadores,
                        Partida partidas[], int cap_partidas, int *qtd_partidas)
{
    int codigo;

    if (conn == NULL || equipes == NULL || qtd_equipes == NULL ||
        jogadores == NULL || qtd_jogadores == NULL ||
        partidas == NULL || qtd_partidas == NULL) {
        return BANCO_ERRO_ARGUMENTO;
    }

    *qtd_equipes = 0;
    *qtd_jogadores = 0;
    *qtd_partidas = 0;

    codigo = carregar_equipes(equipes, cap_equipes, qtd_equipes);
    if (codigo != BANCO_OK) {
        return codigo;
    }

    codigo = carregar_jogadores(jogadores, cap_jogadores, qtd_jogadores);
    if (codigo != BANCO_OK) {
        return codigo;
    }

    return carregar_partidas(partidas, cap_partidas, qtd_partidas);
}
