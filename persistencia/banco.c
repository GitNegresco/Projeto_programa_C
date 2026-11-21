#include "banco.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int banco_resultado_ok(PGconn *conexao, PGresult *resultado,
                       ExecStatusType esperado, const char operacao[])
{
    const char *mensagem;

    if (resultado == NULL) {
        fprintf(stderr, "%s: %s\n", operacao, PQerrorMessage(conexao));
        return 0;
    }

    if (PQresultStatus(resultado) != esperado) {
        mensagem = PQresultErrorMessage(resultado);
        if (mensagem == NULL || mensagem[0] == '\0') {
            mensagem = PQerrorMessage(conexao);
        }
        fprintf(stderr, "%s: %s\n", operacao, mensagem);
        return 0;
    }

    return 1;
}

int banco_executar(PGconn *conexao, const char sql[])
{
    PGresult *resultado = PQexec(conexao, sql);
    int sucesso = banco_resultado_ok(conexao, resultado, PGRES_COMMAND_OK,
                                     "Erro ao executar comando SQL");

    if (resultado != NULL) {
        PQclear(resultado);
    }

    return sucesso;
}

int banco_executar_parametros(PGconn *conexao, const char sql[], int quantidade,
                              const char *parametros[])
{
    PGresult *resultado = PQexecParams(conexao, sql, quantidade, NULL,
                                       parametros, NULL, NULL, 0);
    int sucesso = banco_resultado_ok(conexao, resultado, PGRES_COMMAND_OK,
                                     "Erro ao salvar dados no PostgreSQL");

    if (resultado != NULL) {
        PQclear(resultado);
    }

    return sucesso;
}

int banco_proximo_id(PGconn *conexao, const char tabela[], const char coluna[],
                     int *id)
{
    char sql[100];
    PGresult *resultado;

    snprintf(sql, sizeof(sql),
             "SELECT COALESCE(MAX(%s), 0) + 1 FROM %s", coluna, tabela);
    resultado = PQexec(conexao, sql);

    if (!banco_resultado_ok(conexao, resultado, PGRES_TUPLES_OK,
                            "Erro ao obter o proximo ID")) {
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return 0;
    }

    *id = atoi(PQgetvalue(resultado, 0, 0));
    PQclear(resultado);
    return 1;
}

int banco_conectar(PGconn **conexao)
{
    char *url = getenv("DATABASE_URL");

    if (url == NULL || url[0] == '\0') {
        fprintf(stderr, "Defina a variavel de ambiente DATABASE_URL.\n");
        return 0;
    }

    *conexao = PQconnectdb(url);
    if (*conexao == NULL || PQstatus(*conexao) != CONNECTION_OK) {
        if (*conexao != NULL) {
            fprintf(stderr, "Falha na conexao PostgreSQL: %s\n",
                    PQerrorMessage(*conexao));
            PQfinish(*conexao);
            *conexao = NULL;
        } else {
            fprintf(stderr, "Nao foi possivel iniciar a conexao PostgreSQL.\n");
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
    PGresult *resultado = PQexec(
        conexao,
        "SELECT id_equipe, COALESCE(nome_equipe, ''), "
        "COALESCE(num_jogadores, 0) FROM equipes ORDER BY id_equipe");
    int linhas;
    int i;

    if (!banco_resultado_ok(conexao, resultado, PGRES_TUPLES_OK,
                            "Erro ao carregar equipes")) {
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return 0;
    }

    linhas = PQntuples(resultado);
    if (linhas > capacidade) {
        fprintf(stderr, "O banco tem mais equipes que o limite de %d.\n",
                capacidade);
        PQclear(resultado);
        return 0;
    }

    for (i = 0; i < linhas; i++) {
        equipes[i].id_equipe = atoi(PQgetvalue(resultado, i, 0));
        snprintf(equipes[i].nome_equipe, sizeof(equipes[i].nome_equipe),
                 "%s", PQgetvalue(resultado, i, 1));
        equipes[i].num_jogadores = atoi(PQgetvalue(resultado, i, 2));
    }

    *quantidade = linhas;
    PQclear(resultado);
    return 1;
}

int banco_converter_placar(const char texto[], int *placar)
{
    char *fim;
    double valor = strtod(texto, &fim);

    if (fim == texto || *fim != '\0' || valor != valor ||
        valor < 0 || valor > INT_MAX) {
        return 0;
    }
    if (valor != (int)valor) {
        return 0;
    }

    *placar = (int)valor;
    return 1;
}

int banco_carregar_partidas(PGconn *conexao, Partida partidas[],
                            int capacidade, int *quantidade)
{
    PGresult *resultado = PQexec(
        conexao,
        "SELECT p.id_partida, p.num_rodada, "
        "p.fk_campeonato_id_campeonato, p.fk_jogos_id_jogo, "
        "r.fk_equipes_id_equipe, r.resultado_partida, "
        "COUNT(r.id_resultado) OVER (PARTITION BY p.id_partida) "
        "FROM partidas p LEFT JOIN resultados r "
        "ON r.fk_partidas_id_partida = p.id_partida "
        "ORDER BY p.id_partida, r.fk_equipes_id_equipe");
    int linhas;
    int quantidade_partidas = 0;
    int resultados_na_partida = 0;
    int id_partida_atual = 0;
    int tem_partida_atual = 0;
    int i;

    if (!banco_resultado_ok(conexao, resultado, PGRES_TUPLES_OK,
                            "Erro ao carregar partidas e resultados")) {
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return 0;
    }

    linhas = PQntuples(resultado);
    for (i = 0; i < linhas; i++) {
        int id_partida = atoi(PQgetvalue(resultado, i, 0));

        if (!tem_partida_atual || id_partida != id_partida_atual) {
            if (tem_partida_atual && resultados_na_partida != 2) {
                fprintf(stderr,
                        "A partida %d precisa ter exatamente dois resultados.\n",
                        id_partida_atual);
                PQclear(resultado);
                return 0;
            }
            if (quantidade_partidas >= capacidade) {
                fprintf(stderr, "O banco tem mais partidas que o limite de %d.\n",
                        capacidade);
                PQclear(resultado);
                return 0;
            }

            id_partida_atual = id_partida;
            tem_partida_atual = 1;
            resultados_na_partida = 0;
            partidas[quantidade_partidas].id_partida = id_partida;
            partidas[quantidade_partidas].rodada =
                atoi(PQgetvalue(resultado, i, 1));
            partidas[quantidade_partidas].id_campeonato =
                atoi(PQgetvalue(resultado, i, 2));

            if (PQgetisnull(resultado, i, 1) ||
                PQgetisnull(resultado, i, 2) ||
                PQgetisnull(resultado, i, 3)) {
                fprintf(stderr, "A partida %d tem dados incompletos.\n",
                        id_partida);
                PQclear(resultado);
                return 0;
            }
            partidas[quantidade_partidas].id_jogo =
                atoi(PQgetvalue(resultado, i, 3));

            if (atoi(PQgetvalue(resultado, i, 6)) != 2) {
                fprintf(stderr,
                        "A partida %d precisa ter exatamente dois resultados.\n",
                        id_partida);
                PQclear(resultado);
                return 0;
            }
            quantidade_partidas++;
        }

        if (PQgetisnull(resultado, i, 4) ||
            PQgetisnull(resultado, i, 5) ||
            !banco_converter_placar(PQgetvalue(resultado, i, 5),
                resultados_na_partida == 0
                    ? &partidas[quantidade_partidas - 1].placarA
                    : &partidas[quantidade_partidas - 1].placarB)) {
            fprintf(stderr, "Resultado invalido na partida %d.\n", id_partida);
            PQclear(resultado);
            return 0;
        }

        if (resultados_na_partida == 0) {
            partidas[quantidade_partidas - 1].id_equipeA =
                atoi(PQgetvalue(resultado, i, 4));
        } else if (resultados_na_partida == 1) {
            partidas[quantidade_partidas - 1].id_equipeB =
                atoi(PQgetvalue(resultado, i, 4));
            if (partidas[quantidade_partidas - 1].id_equipeA ==
                partidas[quantidade_partidas - 1].id_equipeB) {
                fprintf(stderr,
                        "A partida %d esta associada duas vezes a mesma equipe.\n",
                        id_partida);
                PQclear(resultado);
                return 0;
            }
        } else {
            fprintf(stderr,
                    "A partida %d tem mais de dois resultados.\n", id_partida);
            PQclear(resultado);
            return 0;
        }

        resultados_na_partida++;
    }

    if (tem_partida_atual && resultados_na_partida != 2) {
        fprintf(stderr, "A partida %d precisa ter exatamente dois resultados.\n",
                id_partida_atual);
        PQclear(resultado);
        return 0;
    }

    *quantidade = quantidade_partidas;
    PQclear(resultado);
    return 1;
}

int banco_salvar_equipe(PGconn *conexao, char nome[], int num_jogadores,
                        int *id_equipe)
{
    char id_texto[32];
    char jogadores_texto[32];
    const char *parametros[3];

    if (!banco_executar(conexao, "BEGIN")) {
        return 0;
    }
    if (!banco_executar(conexao, "LOCK TABLE equipes IN EXCLUSIVE MODE")) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    if (!banco_proximo_id(conexao, "equipes", "id_equipe", id_equipe)) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    snprintf(id_texto, sizeof(id_texto), "%d", *id_equipe);
    snprintf(jogadores_texto, sizeof(jogadores_texto), "%d", num_jogadores);
    parametros[0] = id_texto;
    parametros[1] = nome;
    parametros[2] = jogadores_texto;

    if (!banco_executar_parametros(
            conexao,
            "INSERT INTO equipes (id_equipe, nome_equipe, num_jogadores) "
            "VALUES ($1, $2, $3)",
            3, parametros)) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    if (!banco_executar(conexao, "COMMIT")) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    return 1;
}

int banco_salvar_partida(PGconn *conexao, Partida *partida)
{
    char id_partida[32];
    char id_resultadoA[32];
    char id_resultadoB[32];
    char rodada[32];
    char id_campeonato[32];
    char id_jogo[32];
    char id_equipeA[32];
    char id_equipeB[32];
    char placarA[32];
    char placarB[32];
    const char *parametros_partida[4];
    const char *parametros_resultado[4];
    int proximo_id_resultado;

    if (!banco_executar(conexao, "BEGIN")) {
        return 0;
    }
    if (!banco_executar(conexao, "LOCK TABLE partidas IN EXCLUSIVE MODE") ||
        !banco_executar(conexao, "LOCK TABLE resultados IN EXCLUSIVE MODE")) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    if (!banco_proximo_id(conexao, "partidas", "id_partida",
                          &partida->id_partida) ||
        !banco_proximo_id(conexao, "resultados", "id_resultado",
                          &proximo_id_resultado)) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    snprintf(id_partida, sizeof(id_partida), "%d", partida->id_partida);
    snprintf(rodada, sizeof(rodada), "%d", partida->rodada);
    snprintf(id_campeonato, sizeof(id_campeonato), "%d",
             partida->id_campeonato);
    snprintf(id_jogo, sizeof(id_jogo), "%d", partida->id_jogo);
    parametros_partida[0] = id_partida;
    parametros_partida[1] = rodada;
    parametros_partida[2] = id_campeonato;
    parametros_partida[3] = id_jogo;

    if (!banco_executar_parametros(
            conexao,
            "INSERT INTO partidas (id_partida, num_rodada, "
            "fk_campeonato_id_campeonato, fk_jogos_id_jogo) "
            "VALUES ($1, $2, $3, $4)",
            4, parametros_partida)) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    snprintf(id_resultadoA, sizeof(id_resultadoA), "%d", proximo_id_resultado);
    snprintf(id_resultadoB, sizeof(id_resultadoB), "%d", proximo_id_resultado + 1);
    snprintf(id_equipeA, sizeof(id_equipeA), "%d", partida->id_equipeA);
    snprintf(id_equipeB, sizeof(id_equipeB), "%d", partida->id_equipeB);
    snprintf(placarA, sizeof(placarA), "%d", partida->placarA);
    snprintf(placarB, sizeof(placarB), "%d", partida->placarB);
    parametros_resultado[0] = id_resultadoA;
    parametros_resultado[1] = placarA;
    parametros_resultado[2] = id_equipeA;
    parametros_resultado[3] = id_partida;

    if (!banco_executar_parametros(
            conexao,
            "INSERT INTO resultados (id_resultado, resultado_partida, "
            "fk_equipes_id_equipe, fk_partidas_id_partida) "
            "VALUES ($1, $2, $3, $4)",
            4, parametros_resultado)) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    parametros_resultado[0] = id_resultadoB;
    parametros_resultado[1] = placarB;
    parametros_resultado[2] = id_equipeB;
    if (!banco_executar_parametros(
            conexao,
            "INSERT INTO resultados (id_resultado, resultado_partida, "
            "fk_equipes_id_equipe, fk_partidas_id_partida) "
            "VALUES ($1, $2, $3, $4)",
            4, parametros_resultado)) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    if (!banco_executar(conexao, "COMMIT")) {
        banco_executar(conexao, "ROLLBACK");
        return 0;
    }

    return 1;
}

int banco_listar_campeonatos(PGconn *conexao)
{
    PGresult *resultado = PQexec(
        conexao,
        "SELECT id_campeonato, COALESCE(nome_campeonato, '') "
        "FROM campeonato ORDER BY id_campeonato");
    int linhas;
    int i;

    if (!banco_resultado_ok(conexao, resultado, PGRES_TUPLES_OK,
                            "Erro ao carregar campeonatos")) {
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return -1;
    }

    linhas = PQntuples(resultado);
    printf("\n--- CAMPEONATOS ---\n");
    for (i = 0; i < linhas; i++) {
        printf("ID %s | %s\n", PQgetvalue(resultado, i, 0),
               PQgetvalue(resultado, i, 1));
    }
    PQclear(resultado);
    return linhas;
}

int banco_listar_jogos(PGconn *conexao)
{
    PGresult *resultado = PQexec(
        conexao,
        "SELECT id_jogo, COALESCE(nome, '') FROM jogos ORDER BY id_jogo");
    int linhas;
    int i;

    if (!banco_resultado_ok(conexao, resultado, PGRES_TUPLES_OK,
                            "Erro ao carregar jogos")) {
        if (resultado != NULL) {
            PQclear(resultado);
        }
        return -1;
    }

    linhas = PQntuples(resultado);
    printf("\n--- JOGOS ---\n");
    for (i = 0; i < linhas; i++) {
        printf("ID %s | %s\n", PQgetvalue(resultado, i, 0),
               PQgetvalue(resultado, i, 1));
    }
    PQclear(resultado);
    return linhas;
}