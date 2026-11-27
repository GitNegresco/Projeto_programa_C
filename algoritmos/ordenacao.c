#include "ordenacao.h"

static int equipe_depois(Classificacao equipeA, Classificacao equipeB)
{
    double saldoA = equipeA.pontos_pro - equipeA.pontos_contra;
    double saldoB = equipeB.pontos_pro - equipeB.pontos_contra;

    if (equipeA.pontos != equipeB.pontos) {
        return equipeA.pontos < equipeB.pontos;
    }
    if (equipeA.vitorias != equipeB.vitorias) {
        return equipeA.vitorias < equipeB.vitorias;
    }
    if (saldoA != saldoB) {
        return saldoA < saldoB;
    }
    if (equipeA.pontos_pro != equipeB.pontos_pro) {
        return equipeA.pontos_pro < equipeB.pontos_pro;
    }
    return equipeA.id_equipe > equipeB.id_equipe;
}

void ordenar_equipes_por_id(Equipe equipes[], int qtd_equipes)
{
    int i;
    int j;

    for (i = 0; i < qtd_equipes - 1; i++) {
        for (j = 0; j < qtd_equipes - i - 1; j++) {
            if (equipes[j].id_equipe > equipes[j + 1].id_equipe) {
                Equipe temporaria = equipes[j];
                equipes[j] = equipes[j + 1];
                equipes[j + 1] = temporaria;
            }
        }
    }
}

void ordenar_ranking(Classificacao classificacao[], int quantidade)
{
    int i;
    int j;

    for (i = 0; i < quantidade - 1; i++) {
        for (j = 0; j < quantidade - i - 1; j++) {
            if (equipe_depois(classificacao[j], classificacao[j + 1])) {
                Classificacao temporaria = classificacao[j];
                classificacao[j] = classificacao[j + 1];
                classificacao[j + 1] = temporaria;
            }
        }
    }
}