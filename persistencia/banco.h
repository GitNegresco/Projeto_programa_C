#ifndef BANCO_H
#define BANCO_H
#include "../modelos/entidade.h"

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

#endif
