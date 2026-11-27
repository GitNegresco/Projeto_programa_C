#include <libpq-fe.h>

typedef struct {
    int id_equipe;
    char nome_equipe[101];
    int num_jogadores;
} Equipe;

typedef struct {
    int id_partida;
    int id_equipeA;
    int id_equipeB;
    int placarA;
    int placarB;
    int rodada;
    int id_campeonato;
    int id_jogo;
} Partida;

int banco_conectar(PGconn **conexao);
void banco_desconectar(PGconn *conexao);
int banco_carregar_equipes(PGconn *conexao, Equipe equipes[],
                           int capacidade, int *quantidade);
int banco_carregar_partidas(PGconn *conexao, Partida partidas[],
                            int capacidade, int *quantidade);
int banco_salvar_equipe(PGconn *conexao, char nome[], int num_jogadores,
                        int *id_equipe);
int banco_salvar_partida(PGconn *conexao, Partida *partida);
int banco_listar_campeonatos(PGconn *conexao);
int banco_listar_jogos(PGconn *conexao);
