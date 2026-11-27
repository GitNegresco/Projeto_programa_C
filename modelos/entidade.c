#include <stdio.h>
#include "entidade.h"

void listar_jogadores(Jogador jogadores[], int qtd_jogadores) {
    if (qtd_jogadores == 0) {
        printf("Não há jogadores cadastrados.\n");
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
        printf("Não há equipes cadastradas.\n");
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

static int buscar_indice_jogador(Equipe equipes[], int qtd_equipes, int id_equipe) {
    for (int i = 0; i < qtd_equipes; i++) {
        if (equipes[i].id_equipe == id_equipe) {
            return i;
        }
    }
    return ERRO_NAO_ENCONTRADO;
}