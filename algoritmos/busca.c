
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

