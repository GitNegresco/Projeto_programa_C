#ifndef BANCO_H
#define BANCO_H

#include <libpq-fe.h>
#include "../modelos/entidade.h"

int banco_conectar(PGconn **conexao);
void banco_desconectar(PGconn *conexao);
int banco_carregar_equipes(PGconn *conexao, Equipe equipes[],
                           int capacidade, int *quantidade);
int banco_carregar_jogadores(PGconn *conexao, Jogador jogadores[],
                             int capacidade, int *quantidade);
int banco_carregar_partidas(PGconn *conexao, Partida partidas[],
                            int capacidade, int *quantidade);
int banco_salvar_equipe(PGconn *conexao, const char nome[],
                        int *id_equipe);
int banco_salvar_partida(PGconn *conexao, Partida *partida);
int banco_listar_campeonatos(PGconn *conexao);
int banco_listar_jogos(PGconn *conexao);

#endif
