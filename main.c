#include <stdio.h>

#include "persistencia/banco.h"

typedef struct {
    int id_equipe;
    char nome_equipe[101];
    int pontos;
    int vitorias;
    int empates;
    int derrotas;
    int jogos;
    int pontos_pro;
    int pontos_contra;
} Classificacao;

int ler_inteiro(char mensagem[], int *valor)
{
    int caractere;

    while (1) {
        printf("%s", mensagem);
        if (scanf("%d", valor) != 1) {
            while ((caractere = getchar()) != '\n' && caractere != EOF) {
            }
            if (caractere == EOF) {
                return 0;
            }
            printf("Entrada invalida. Digite um numero inteiro.\n");
        } else {
            while ((caractere = getchar()) != '\n' && caractere != EOF) {
            }
            return 1;
        }
    }
}

int nomes_iguais(char nomeA[], char nomeB[])
{
    int i = 0;

    while (nomeA[i] != '\0' && nomeB[i] != '\0') {
        if (nomeA[i] != nomeB[i]) {
            return 0;
        }
        i++;
    }

    return nomeA[i] == nomeB[i];
}

int buscar_equipe_binaria(Equipe equipes[], int quantidade, int id)
{
    int inicio = 0;
    int fim = quantidade - 1;

    while (inicio <= fim) {
        int meio = (inicio + fim) / 2;

        if (equipes[meio].id_equipe == id) {
            return meio;
        }
        if (equipes[meio].id_equipe < id) {
            inicio = meio + 1;
        } else {
            fim = meio - 1;
        }
    }

    return -1;
}

int cadastrar_equipe_menu(PGconn *conexao, Equipe equipes[],
                          int *quantidade)
{
    char nome[101];
    int num_jogadores;
    int id_equipe;
    int i;
    int caractere;

    if (*quantidade == 100) {
        printf("Limite de equipes atingido.\n");
        return 1;
    }

    printf("Nome da equipe (sem espacos): ");
    if (scanf("%100s", nome) != 1) {
        return 0;
    }
    while ((caractere = getchar()) != '\n' && caractere != EOF) {
    }

    for (i = 0; i < *quantidade; i++) {
        if (nomes_iguais(equipes[i].nome_equipe, nome)) {
            printf("Ja existe uma equipe com esse nome.\n");
            return 1;
        }
    }

    if (!ler_inteiro("Numero de jogadores: ", &num_jogadores)) {
        return 0;
    }
    if (num_jogadores < 0) {
        printf("O numero de jogadores nao pode ser negativo.\n");
        return 1;
    }

    if (!banco_salvar_equipe(conexao, nome, num_jogadores, &id_equipe)) {
        return 1;
    }

    equipes[*quantidade].id_equipe = id_equipe;
    equipes[*quantidade].num_jogadores = num_jogadores;
    i = 0;
    while (nome[i] != '\0') {
        equipes[*quantidade].nome_equipe[i] = nome[i];
        i++;
    }
    equipes[*quantidade].nome_equipe[i] = '\0';

    printf("Equipe cadastrada no banco com ID %d.\n", id_equipe);
    *quantidade = *quantidade + 1;
    return 1;
}

void listar_equipes_menu(Equipe equipes[], int quantidade)
{
    int i;

    if (quantidade == 0) {
        printf("Nenhuma equipe cadastrada.\n");
        return;
    }

    printf("\n--- EQUIPES ---\n");
    for (i = 0; i < quantidade; i++) {
        printf("ID %d | %s | Jogadores: %d\n",
               equipes[i].id_equipe, equipes[i].nome_equipe,
               equipes[i].num_jogadores);
    }
}

int registrar_partida_menu(PGconn *conexao, Partida partidas[],
                           int *quantidade_partidas, Equipe equipes[],
                           int quantidade_equipes)
{
    Partida partida;
    int idA;
    int idB;
    int resultado;
    int quantidade_campeonatos;
    int quantidade_jogos;

    if (quantidade_equipes < 2) {
        printf("Cadastre pelo menos duas equipes antes de registrar partidas.\n");
        return 1;
    }
    if (*quantidade_partidas == 1000) {
        printf("Limite de partidas atingido.\n");
        return 1;
    }

    quantidade_campeonatos = banco_listar_campeonatos(conexao);
    if (quantidade_campeonatos < 0) {
        return 1;
    }
    if (quantidade_campeonatos == 0) {
        printf("Cadastre um campeonato no banco antes de registrar partidas.\n");
        return 1;
    }

    quantidade_jogos = banco_listar_jogos(conexao);
    if (quantidade_jogos < 0) {
        return 1;
    }
    if (quantidade_jogos == 0) {
        printf("Cadastre um jogo no banco antes de registrar partidas.\n");
        return 1;
    }

    listar_equipes_menu(equipes, quantidade_equipes);
    if (!ler_inteiro("ID do campeonato: ", &partida.id_campeonato)) return 0;
    if (!ler_inteiro("ID do jogo: ", &partida.id_jogo)) return 0;
    if (!ler_inteiro("ID da equipe A: ", &idA)) return 0;
    if (!ler_inteiro("ID da equipe B: ", &idB)) return 0;

    if (buscar_equipe_binaria(equipes, quantidade_equipes, idA) == -1 ||
        buscar_equipe_binaria(equipes, quantidade_equipes, idB) == -1) {
        printf("ID de equipe nao encontrado.\n");
        return 1;
    }
    if (idA == idB) {
        printf("Uma equipe nao pode jogar contra ela mesma.\n");
        return 1;
    }

    partida.id_equipeA = idA;
    partida.id_equipeB = idB;
    if (!ler_inteiro("Placar da equipe A: ", &partida.placarA)) return 0;
    if (!ler_inteiro("Placar da equipe B: ", &partida.placarB)) return 0;
    if (!ler_inteiro("Rodada: ", &partida.rodada)) return 0;

    if (partida.placarA < 0 || partida.placarB < 0 || partida.rodada < 1) {
        printf("Os placares nao podem ser negativos e a rodada deve ser maior que zero.\n");
        return 1;
    }

    if (!banco_salvar_partida(conexao, &partida)) {
        return 1;
    }

    partidas[*quantidade_partidas] = partida;
    *quantidade_partidas = *quantidade_partidas + 1;
    printf("Partida registrada no banco com ID %d.\n", partida.id_partida);
    return 1;
}

int montar_classificacao(Equipe equipes[], int quantidade_equipes,
                        Partida partidas[], int quantidade_partidas,
                        Classificacao classificacao[])
{
    int i;
    int j;

    for (i = 0; i < quantidade_equipes; i++) {
        classificacao[i].id_equipe = equipes[i].id_equipe;
        for (j = 0; j < 101; j++) {
            classificacao[i].nome_equipe[j] = equipes[i].nome_equipe[j];
            if (equipes[i].nome_equipe[j] == '\0') {
                break;
            }
        }
        classificacao[i].pontos = 0;
        classificacao[i].vitorias = 0;
        classificacao[i].empates = 0;
        classificacao[i].derrotas = 0;
        classificacao[i].jogos = 0;
        classificacao[i].pontos_pro = 0;
        classificacao[i].pontos_contra = 0;
    }

    for (i = 0; i < quantidade_partidas; i++) {
        int equipeA = buscar_equipe_binaria(equipes, quantidade_equipes,
                                           partidas[i].id_equipeA);
        int equipeB = buscar_equipe_binaria(equipes, quantidade_equipes,
                                           partidas[i].id_equipeB);

        if (equipeA == -1 || equipeB == -1) {
            continue;
        }

        classificacao[equipeA].jogos++;
        classificacao[equipeB].jogos++;
        classificacao[equipeA].pontos_pro += partidas[i].placarA;
        classificacao[equipeA].pontos_contra += partidas[i].placarB;
        classificacao[equipeB].pontos_pro += partidas[i].placarB;
        classificacao[equipeB].pontos_contra += partidas[i].placarA;

        if (partidas[i].placarA > partidas[i].placarB) {
            classificacao[equipeA].vitorias++;
            classificacao[equipeA].pontos += 3;
            classificacao[equipeB].derrotas++;
        } else if (partidas[i].placarA < partidas[i].placarB) {
            classificacao[equipeB].vitorias++;
            classificacao[equipeB].pontos += 3;
            classificacao[equipeA].derrotas++;
        } else {
            classificacao[equipeA].empates++;
            classificacao[equipeB].empates++;
            classificacao[equipeA].pontos += 1;
            classificacao[equipeB].pontos += 1;
        }
    }

    return quantidade_equipes;
}

int equipe_depois(Classificacao equipeA, Classificacao equipeB)
{
    int saldoA = equipeA.pontos_pro - equipeA.pontos_contra;
    int saldoB = equipeB.pontos_pro - equipeB.pontos_contra;

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

void ordenar_bubble(Classificacao classificacao[], int quantidade)
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

void exibir_ranking(Equipe equipes[], int quantidade_equipes,
                    Partida partidas[], int quantidade_partidas)
{
    Classificacao classificacao[100];
    int quantidade = montar_classificacao(equipes, quantidade_equipes,
                                          partidas, quantidade_partidas,
                                          classificacao);
    int i;

    if (quantidade == 0) {
        printf("Cadastre equipes para visualizar o ranking.\n");
        return;
    }

    ordenar_bubble(classificacao, quantidade);
    printf("\n================ RANKING ================\n");
    printf("POS ID  EQUIPE              PTS J  V  E  D  SALDO\n");
    for (i = 0; i < quantidade; i++) {
        int saldo = classificacao[i].pontos_pro -
                    classificacao[i].pontos_contra;

        printf("%3d %2d  %-18s %3d %2d %2d %2d %2d %6d\n",
               i + 1, classificacao[i].id_equipe,
               classificacao[i].nome_equipe, classificacao[i].pontos,
               classificacao[i].jogos, classificacao[i].vitorias,
               classificacao[i].empates, classificacao[i].derrotas, saldo);
    }
}

void buscar_equipe_por_id_menu(Equipe equipes[], int quantidade)
{
    int id;
    int indice;

    if (!ler_inteiro("Digite o ID da equipe: ", &id)) {
        return;
    }

    indice = buscar_equipe_binaria(equipes, quantidade, id);
    if (indice == -1) {
        printf("Equipe nao encontrada.\n");
    } else {
        printf("Equipe encontrada: ID %d | %s\n",
               equipes[indice].id_equipe, equipes[indice].nome_equipe);
    }
}

int main(void)
{
    PGconn *conexao = NULL;
    Equipe equipes[100];
    Partida partidas[1000];
    int quantidade_equipes = 0;
    int quantidade_partidas = 0;
    int opcao = -1;

    if (!banco_conectar(&conexao)) {
        return 1;
    }
    if (!banco_carregar_equipes(conexao, equipes, 100, &quantidade_equipes) ||
        !banco_carregar_partidas(conexao, partidas, 1000,
                                 &quantidade_partidas)) {
        banco_desconectar(conexao);
        return 1;
    }

    printf("Conectado ao banco. Equipes e partidas carregadas.\n");
    printf("CAMPEONATO DE E-SPORTS\n");

    while (opcao != 0) {
        printf("\n1. Cadastrar equipe\n");
        printf("2. Listar equipes\n");
        printf("3. Registrar partida\n");
        printf("4. Exibir ranking\n");
        printf("5. Buscar equipe por ID\n");
        printf("0. Sair\n");

        if (!ler_inteiro("Escolha uma opcao: ", &opcao)) {
            break;
        }

        if (opcao == 1) {
            if (!cadastrar_equipe_menu(conexao, equipes, &quantidade_equipes)) {
                break;
            }
        } else if (opcao == 2) {
            listar_equipes_menu(equipes, quantidade_equipes);
        } else if (opcao == 3) {
            if (!registrar_partida_menu(conexao, partidas,
                                        &quantidade_partidas, equipes,
                                        quantidade_equipes)) {
                break;
            }
        } else if (opcao == 4) {
            exibir_ranking(equipes, quantidade_equipes,
                           partidas, quantidade_partidas);
        } else if (opcao == 5) {
            buscar_equipe_por_id_menu(equipes, quantidade_equipes);
        } else if (opcao == 0) {
            printf("Encerrando o campeonato.\n");
        } else {
            printf("Opcao invalida.\n");
        }
    }

    banco_desconectar(conexao);
    return 0;
}
