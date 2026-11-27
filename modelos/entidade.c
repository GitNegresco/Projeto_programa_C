#include <stdio.h>
#include <string.h>
#include "entidade.h"

/*TODO: trocar pelo banco real*/
int banco_inserir_jogador(char nick[50]) {
    static int id = 0;
    id++;
    return id;
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
            if (equipes[i].status_equipe == ATIVA) {
                printf("%d: %s - Ativa.\n", equipes[i].id_equipe, equipes[i].nome_equipe);
            } else if (equipes[i].status_equipe == DESCLASSIFICADA) {
                printf("%d: %s - Desclassificada.\n", equipes[i].id_equipe, equipes[i].nome_equipe);
            } else {
                printf("%d: %s - Status desconhecido.\n", equipes[i].id_equipe, equipes[i].nome_equipe);
            }
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

static int buscar_indice_equipe(Equipe equipes[], int qtd_equipes, int id_equipe) {
    for (int i = 0; i < qtd_equipes; i++) {
        if (equipes[i].id_equipe == id_equipe) {
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
    strncpy(jogadores[posicao].nickname, nick_jogador, 49);
    jogadores[posicao].nickname[49] = '\0';
    jogadores[posicao].id_equipe = SEM_EQUIPE;

    (*qtd_jogadores)++;
    return OK;
}