#ifndef ENTIDADE_H
#define ENTIDADE_H

/*Definição dos valores de status da equipe, e "sem equipe" para o jogador*/
#define SEM_EQUIPE -1
#define DESCLASSIFICADA 0
#define ATIVA 1

/*Definição dos limites dos arrays*/
#define MAX_JOGADORES 500
#define MAX_EQUIPES 100
#define MAX_PARTIDAS 1000

/*Definição das pontuações*/
#define PONTOS_VITORIA 3
#define PONTOS_EMPATE 1
#define PONTOS_DERROTA 0

/*Definição dos códigos de retorno*/
#define OK 0
#define ERRO_DUPLICADO -1
#define ERRO_CHEIO -2
#define ERRO_NAO_ENCONTRADO -3
#define ERRO_INVALIDO -4
#define ERRO_BLOQUEADO -5
#define ERRO_BANCO -6

typedef struct {
    int id_jogador;
    char nickname[50];
    int id_equipe;
} Jogador;

typedef struct {
    int id_equipe;
    char nome_equipe[50];
    int status_equipe;
} Equipe;

typedef struct {
    int placarA;
    int placarB;
    int id_equipeA;
    int id_equipeB;
    int rodada;
    int id_partida;
} Partida;

int cadastrar_jogador(Jogador jogadores[], int *qtd_jogadores, char nick_jogador[50]);
int remover_jogador(Jogador jogadores[], int *qtd_jogadores, int id_jogador);

int cadastrar_equipe(Equipe equipes[], int *qtd_equipes, char nome_equipe[50]);
int remover_equipe(Equipe equipes[], int *qtd_equipes, int id_equipe, Jogador jogadores[], int qtd_jogadores, Partida partidas[], int qtd_partidas);

int adicionar_jogador_equipe(Jogador jogadores[], int qtd_jogadores, int id_jogador, Equipe equipes[], int qtd_equipes, int id_equipe);

int registrar_partida(Partida partidas[], Equipe equipes[], int id_equipeA, int id_equipeB, int placar_equipeA, int placar_equipeB, int rodada);

void listar_jogadores(Jogador jogadores[], int qtd_jogadores);
void listar_equipes(Equipe equipes[], int qtd_equipes);
int calcular_pontuacao(int id_equipe, Partida partidas[], int qtd_partidas);

#endif