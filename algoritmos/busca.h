
#ifndef BUSCA_H
#define BUSCA_H

#include "../modelos/entidade.h"

int buscar_jogador_por_nickname(
    const Jogador jogadores[],
    int qtd_jogadores,
    const char *nickname
);

int buscar_equipe_por_nome(
    const Equipe equipes[],
    int qtd_equipes,
    const char *nome_equipe
);

int buscar_equipe_por_id(
    const Equipe equipes[],
    int qtd_equipes,
    int id_equipe
);

#endif
