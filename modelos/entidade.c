#include <stdio.h>
#include <string.h>
#include "entidade.h"

/*TODO: trocar pelo banco real*/
int banco_inserir_jogador(char nick[50]) {
    static int id = 0;
    (void)nick;
    id++;
    return id;
}

/*TODO: trocar pelo banco real*/
int banco_remover_jogador(int id) {
    (void)id;
    return OK;
}

void listar_jogadores(Jogador jogadores[], int qtd_jogadores) {
    if (qtd_jogadores == 0) {
        printf("Nao ha jogadores cadastrados.\n");
    } else {
        for (int i = 0; i < qtd_jogadores; i++) {
            if (jogadores[i].id_equipe != SEM_EQUIPE) {
                printf("%d: %s - Equipe %d.\n",  jogadores[i].id_jogador, jogadores[i].nickname, jogadores[i].id_equipe);
            } else {
                printf("%d: %s - Sem equipe.\n", jogadores[i].id_jogador, jogadores[i].nickname);
            }
        }
    }
}

void listar_equipes(Equipe equipes[], int qtd_equipes) {
    if (qtd_equipes == 0) {
        printf("Nao ha equipes cadastradas.\n");
    } else {
        for (int i = 0; i < qtd_equipes; i++) {
            printf("%d: %s - %d jogador(es).\n",
                   equipes[i].id_equipe, equipes[i].nome_equipe,
                   equipes[i].num_jogadores);
        }
    }
}

static int buscar_indice_jogador(Jogador jogadores[], int qtd_jogadores, int id_jogador) {
    for (int i = 0; i < qtd_jogadores; i++) {
        if (jogadores[i].id_jogador == id_jogador) {
            return i;
        }
    }
    return ERRO_NAO_ENCONTRADO;
}

int cadastrar_jogador(Jogador jogadores[], int *qtd_jogadores, char nick_jogador[50]) {

    int posicao = *qtd_jogadores;
    if (posicao >= MAX_JOGADORES) {
        return ERRO_CHEIO;
    } else if (nick_jogador[0] == '\0' ) {
        return ERRO_INVALIDO;
    }
    for (int i = 0; i < posicao; i++) {
        if (strcmp(nick_jogador, jogadores[i].nickname) == 0) {
            return ERRO_DUPLICADO;
        }
    }

    int id = banco_inserir_jogador(nick_jogador);
    if (id < 0) {
        return ERRO_BANCO;
    }

    jogadores[posicao].id_jogador = id;
    jogadores[posicao].nome_completo[0] = '\0';
    strncpy(jogadores[posicao].nickname, nick_jogador, 49);
    jogadores[posicao].nickname[49] = '\0';
    jogadores[posicao].id_equipe = SEM_EQUIPE;

    (*qtd_jogadores)++;
    return OK;
}

int remover_jogador(Jogador jogadores[], int *qtd_jogadores, int id_jogador) {
    int posicao = buscar_indice_jogador(jogadores, *qtd_jogadores, id_jogador);
    if (posicao == ERRO_NAO_ENCONTRADO) {
        return ERRO_NAO_ENCONTRADO;
    } else if (banco_remover_jogador(id_jogador) != OK) {
        return ERRO_BANCO;
    }

    for (int i = posicao; i < *qtd_jogadores-1; i++) {
        jogadores[i] = jogadores[i+1];
    }
    (*qtd_jogadores)--;
    return OK;
}

int calcular_pontuacao(int id_equipe, Partida partidas[], int qtd_partidas) {
    int pontos = 0;
    int i;

    if ((partidas == NULL && qtd_partidas > 0) || qtd_partidas < 0) {
        return ERRO_INVALIDO;
    }

    for (i = 0; i < qtd_partidas; i++) {
        if (partidas[i].id_equipeA == id_equipe) {
            if (partidas[i].placarA > partidas[i].placarB) {
                pontos += PONTOS_VITORIA;
            } else if (partidas[i].placarA == partidas[i].placarB) {
                pontos += PONTOS_EMPATE;
            }
        } else if (partidas[i].id_equipeB == id_equipe) {
            if (partidas[i].placarB > partidas[i].placarA) {
                pontos += PONTOS_VITORIA;
            } else if (partidas[i].placarA == partidas[i].placarB) {
                pontos += PONTOS_EMPATE;
            }
        }
    }

    return pontos;
}