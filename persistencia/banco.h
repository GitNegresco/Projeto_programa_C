#ifndef BANCO_H
#define BANCO_H
#include "../modelos/entidade.h"

<<<<<<< HEAD
#define BANCO_OK                    0
#define BANCO_ERRO_ARGUMENTO       -1  /* parametro inválido ou sem conexao */
#define BANCO_ERRO_BANCO           -2  /* o PostgreSQL recusou o comando */
#define BANCO_ERRO_NAO_ENCONTRADO  -3  /* id inexistente (nenhuma linha afetada nessa situação) */

int  banco_conectar(void);      /* se 0 = ok, se negativo = falhou */
void banco_desconectar(void);

int banco_inserir_jogador(const char *nick);   /* se id > 0, negativo = falhou */
int banco_inserir_equipe(const char *nome);
int banco_inserir_partida(int idA, int idB, int placarA, int placarB, int rodada);

int banco_remover_jogador(int id);             /* se 0 = ok, se negativo = falhou */
int banco_remover_equipe(int id);
int banco_atualizar_equipe_jogador(int id_jogador, int id_equipe);

/* carga inicial: a assinatura depende dos arrays e contadores do entidade.h */
int banco_carregar_tudo(/* ... */);
=======
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
int banco_salvar_jogador(PGconn *conexao, int cpf,
                         const char nome_completo[], const char apelido[],
                         const char conta_steam[], const char telefone[],
                         int id_equipe);
int banco_salvar_partida(PGconn *conexao, Partida *partida);
int banco_listar_campeonatos(PGconn *conexao);
int banco_listar_jogos(PGconn *conexao);
>>>>>>> c1570ca (arrumei uns baguio)

#endif
