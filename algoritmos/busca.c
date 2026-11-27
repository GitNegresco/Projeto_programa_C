
#include <stdio.h>
#include <string.h>
#include "busca.h"

/* Busca linear de jogador pelo nickname */
int buscar_jogador_por_nickname(
    const Jogador jogadores[],
    int qtd_jogadores,
    const char *nickname
) {
    if (jogadores == NULL || nickname == NULL || qtd_jogadores < 0) {
        return ERRO_INVALIDO;
    }

    for (int i = 0; i < qtd_jogadores; i++) {
        if (strcmp(jogadores[i].nickname, nickname) == 0) {
            return i;
        }
    }

    return ERRO_NAO_ENCONTRADO;
}

/* Busca linear de equipe pelo nome */
int buscar_equipe_por_nome(
    const Equipe equipes[],
    int qtd_equipes,
    const char *nome_equipe
) {
    if (equipes == NULL || nome_equipe == NULL || qtd_equipes < 0) {
        return ERRO_INVALIDO;
    }

    for (int i = 0; i < qtd_equipes; i++) {
        if (strcmp(equipes[i].nome_equipe, nome_equipe) == 0) {
            return i;
        }
    }

    return ERRO_NAO_ENCONTRADO;
}

/* A busca binaria exige que equipes esteja ordenado por id_equipe. */
int buscar_equipe_por_id(
    const Equipe equipes[],
    int qtd_equipes,
    int id_equipe
) {
    int inicio = 0;
    int fim = qtd_equipes - 1;

    if ((equipes == NULL && qtd_equipes > 0) || qtd_equipes < 0) {
        return ERRO_INVALIDO;
    }

    while (inicio <= fim) {
        int meio = inicio + (fim - inicio) / 2;

        if (equipes[meio].id_equipe == id_equipe) {
            return meio;
        }
        if (equipes[meio].id_equipe < id_equipe) {
            inicio = meio + 1;
        } else {
            fim = meio - 1;
        }
    }

    return ERRO_NAO_ENCONTRADO;
}

