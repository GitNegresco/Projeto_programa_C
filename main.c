#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "algoritmos/busca.h"
#include "algoritmos/ordenacao.h"
#include "modelos/entidade.h"
#include "persistencia/banco.h"

static int ler_linha(const char *mensagem, char texto[], size_t capacidade)
{
    size_t tamanho;
    int caractere;

    printf("%s", mensagem);
    if (fgets(texto, (int)capacidade, stdin) == NULL) {
        return 0;
    }

    tamanho = strlen(texto);
    if (tamanho > 0 && texto[tamanho - 1] == '\n') {
        texto[--tamanho] = '\0';
    } else if (!feof(stdin)) {
        while ((caractere = getchar()) != '\n' && caractere != EOF) {
        }
        fprintf(stderr, "Entrada excede o limite permitido.\n");
        return 0;
    }

    if (tamanho > 0 && texto[tamanho - 1] == '\r') {
        texto[--tamanho] = '\0';
    }
    return 1;
}

static int ler_inteiro(const char *mensagem, int *valor)
{
    char texto[128];
    char *fim;
    long numero;

    while (1) {
        if (!ler_linha(mensagem, texto, sizeof(texto))) {
            return 0;
        }
        errno = 0;
        numero = strtol(texto, &fim, 10);
        if (errno == 0 && fim != texto && *fim == '\0' &&
            numero >= -2147483647L - 1 && numero <= 2147483647L) {
            *valor = (int)numero;
            return 1;
        }
        printf("Entrada invalida. Digite um numero inteiro.\n");
    }
}

static int ler_decimal(const char *mensagem, double *valor)
{
    char texto[128];
    char *fim;
    double centavos;

    while (1) {
        if (!ler_linha(mensagem, texto, sizeof(texto))) {
            return 0;
        }
        errno = 0;
        *valor = strtod(texto, &fim);
        centavos = *valor * 100.0;
        if (errno == 0 && fim != texto && *fim == '\0' &&
            isfinite(*valor) && fabs(*valor) <= 99999999.99 &&
            fabs(centavos - floor(centavos + 0.5)) < 1e-8) {
            return 1;
        }
        printf("Digite um numero com ate duas casas decimais.\n");
    }
}

static int cadastrar_equipe_menu(PGconn *conexao, Equipe equipes[],
                                 int *quantidade)
{
    char nome[101];
    int id_equipe;
    int indice;

    if (*quantidade >= MAX_EQUIPES) {
        printf("Limite de equipes atingido.\n");
        return 1;
    }
    if (!ler_linha("Nome da equipe: ", nome, sizeof(nome))) {
        return 0;
    }
    if (nome[0] == '\0') {
        printf("O nome da equipe nao pode ficar vazio.\n");
        return 1;
    }
    indice = buscar_equipe_por_nome(equipes, *quantidade, nome);
    if (indice >= 0) {
        printf("Ja existe uma equipe com esse nome.\n");
        return 1;
    }
    if (indice != ERRO_NAO_ENCONTRADO) {
        fprintf(stderr, "Nao foi possivel validar o nome da equipe.\n");
        return 0;
    }
    if (!banco_salvar_equipe(conexao, nome, &id_equipe)) {
        return 1;
    }

    equipes[*quantidade].id_equipe = id_equipe;
    strcpy(equipes[*quantidade].nome_equipe, nome);
    equipes[*quantidade].num_jogadores = 0;
    (*quantidade)++;
    ordenar_equipes_por_id(equipes, *quantidade);
    printf("Equipe cadastrada no banco com ID %d.\n", id_equipe);
    return 1;
}

static int registrar_partida_menu(PGconn *conexao, Partida partidas[],
                                  int *quantidade_partidas,
                                  Equipe equipes[], int quantidade_equipes)
{
    Partida partida;
    int quantidade_campeonatos;
    int quantidade_jogos;
    int indice_a;
    int indice_b;

    if (quantidade_equipes < 2) {
        printf("Cadastre pelo menos duas equipes antes de registrar partidas.\n");
        return 1;
    }
    if (*quantidade_partidas >= MAX_PARTIDAS) {
        printf("Limite de partidas atingido.\n");
        return 1;
    }

    quantidade_campeonatos = banco_listar_campeonatos(conexao);
    if (quantidade_campeonatos <= 0) {
        if (quantidade_campeonatos == 0) {
            printf("Cadastre um campeonato no banco antes de registrar partidas.\n");
        }
        return 1;
    }
    quantidade_jogos = banco_listar_jogos(conexao);
    if (quantidade_jogos <= 0) {
        if (quantidade_jogos == 0) {
            printf("Cadastre um jogo no banco antes de registrar partidas.\n");
        }
        return 1;
    }

    listar_equipes(equipes, quantidade_equipes);
    if (!ler_inteiro("ID do campeonato: ", &partida.id_campeonato) ||
        !ler_inteiro("ID do jogo: ", &partida.id_jogo) ||
        !ler_inteiro("ID da equipe A: ", &partida.id_equipeA) ||
        !ler_inteiro("ID da equipe B: ", &partida.id_equipeB)) {
        return 0;
    }

    indice_a = buscar_equipe_por_id(equipes, quantidade_equipes,
                                    partida.id_equipeA);
    indice_b = buscar_equipe_por_id(equipes, quantidade_equipes,
                                    partida.id_equipeB);
    if (indice_a < 0 || indice_b < 0) {
        printf("ID de equipe nao encontrado.\n");
        return 1;
    }
    if (partida.id_equipeA == partida.id_equipeB) {
        printf("Uma equipe nao pode jogar contra ela mesma.\n");
        return 1;
    }
    if (!ler_decimal("Placar da equipe A: ", &partida.placarA) ||
        !ler_decimal("Placar da equipe B: ", &partida.placarB) ||
        !ler_inteiro("Rodada: ", &partida.rodada)) {
        return 0;
    }
    if (partida.placarA < 0 || partida.placarB < 0 || partida.rodada < 1) {
        printf("Os placares nao podem ser negativos e a rodada deve ser positiva.\n");
        return 1;
    }

    if (!banco_salvar_partida(conexao, &partida)) {
        return 1;
    }
    partidas[*quantidade_partidas] = partida;
    (*quantidade_partidas)++;
    printf("Partida registrada no banco com ID %d.\n", partida.id_partida);
    return 1;
}

static int montar_classificacao(Equipe equipes[], int quantidade_equipes,
                                Partida partidas[], int quantidade_partidas,
                                Classificacao classificacao[])
{
    int i;
    int j;

    for (i = 0; i < quantidade_equipes; i++) {
        memset(&classificacao[i], 0, sizeof(classificacao[i]));
        classificacao[i].id_equipe = equipes[i].id_equipe;
        strcpy(classificacao[i].nome_equipe, equipes[i].nome_equipe);
        classificacao[i].pontos = calcular_pontuacao(
            equipes[i].id_equipe, partidas, quantidade_partidas);
        if (classificacao[i].pontos < 0) {
            fprintf(stderr, "Falha ao calcular a pontuacao da equipe.\n");
            return 0;
        }
    }

    for (i = 0; i < quantidade_partidas; i++) {
        int equipe_a = buscar_equipe_por_id(
            equipes, quantidade_equipes, partidas[i].id_equipeA);
        int equipe_b = buscar_equipe_por_id(
            equipes, quantidade_equipes, partidas[i].id_equipeB);

        if (equipe_a < 0 || equipe_b < 0) {
            fprintf(stderr,
                    "A partida %d referencia uma equipe que nao foi carregada.\n",
                    partidas[i].id_partida);
            return 0;
        }

        classificacao[equipe_a].jogos++;
        classificacao[equipe_b].jogos++;
        classificacao[equipe_a].pontos_pro += partidas[i].placarA;
        classificacao[equipe_a].pontos_contra += partidas[i].placarB;
        classificacao[equipe_b].pontos_pro += partidas[i].placarB;
        classificacao[equipe_b].pontos_contra += partidas[i].placarA;

        if (partidas[i].placarA > partidas[i].placarB) {
            classificacao[equipe_a].vitorias++;
            classificacao[equipe_b].derrotas++;
        } else if (partidas[i].placarB > partidas[i].placarA) {
            classificacao[equipe_b].vitorias++;
            classificacao[equipe_a].derrotas++;
        } else {
            classificacao[equipe_a].empates++;
            classificacao[equipe_b].empates++;
        }
    }

    for (j = 0; j < quantidade_equipes; j++) {
        if (classificacao[j].vitorias + classificacao[j].empates +
            classificacao[j].derrotas != classificacao[j].jogos) {
            fprintf(stderr, "Dados inconsistentes ao montar o ranking.\n");
            return 0;
        }
    }
    return 1;
}

static int exibir_ranking(Equipe equipes[], int quantidade_equipes,
                          Partida partidas[], int quantidade_partidas)
{
    Classificacao classificacao[MAX_EQUIPES];
    int i;

    if (quantidade_equipes == 0) {
        printf("Cadastre equipes para visualizar o ranking.\n");
        return 1;
    }
    if (!montar_classificacao(equipes, quantidade_equipes, partidas,
                              quantidade_partidas, classificacao)) {
        return 0;
    }

    ordenar_ranking(classificacao, quantidade_equipes);
    printf("\n================ RANKING ================\n");
    printf("POS ID  EQUIPE                         PTS  J  V  E  D  SALDO\n");
    for (i = 0; i < quantidade_equipes; i++) {
        double saldo = classificacao[i].pontos_pro -
                       classificacao[i].pontos_contra;
        printf("%3d %3d %-29s %3d %2d %2d %2d %2d %7.2f\n",
               i + 1, classificacao[i].id_equipe,
               classificacao[i].nome_equipe, classificacao[i].pontos,
               classificacao[i].jogos, classificacao[i].vitorias,
               classificacao[i].empates, classificacao[i].derrotas, saldo);
    }
    return 1;
}

static void buscar_equipe_menu(Equipe equipes[], int quantidade)
{
    int id;
    int indice;

    if (!ler_inteiro("Digite o ID da equipe: ", &id)) {
        return;
    }
    indice = buscar_equipe_por_id(equipes, quantidade, id);
    if (indice == ERRO_NAO_ENCONTRADO) {
        printf("Equipe nao encontrada.\n");
    } else if (indice < 0) {
        fprintf(stderr, "Falha ao buscar equipe.\n");
    } else {
        printf("Equipe encontrada: ID %d | %s | %d jogador(es)\n",
               equipes[indice].id_equipe, equipes[indice].nome_equipe,
               equipes[indice].num_jogadores);
    }
}

int main(void)
{
    PGconn *conexao = NULL;
    Equipe equipes[MAX_EQUIPES];
    Jogador jogadores[MAX_JOGADORES];
    Partida partidas[MAX_PARTIDAS];
    int quantidade_equipes = 0;
    int quantidade_jogadores = 0;
    int quantidade_partidas = 0;
    int opcao;
    int continuar = 1;

    if (!banco_conectar(&conexao)) {
        return 1;
    }
    if (!banco_carregar_equipes(conexao, equipes, MAX_EQUIPES,
                                &quantidade_equipes) ||
        !banco_carregar_jogadores(conexao, jogadores, MAX_JOGADORES,
                                  &quantidade_jogadores) ||
        !banco_carregar_partidas(conexao, partidas, MAX_PARTIDAS,
                                 &quantidade_partidas)) {
        banco_desconectar(conexao);
        return 1;
    }
    ordenar_equipes_por_id(equipes, quantidade_equipes);

    printf("Conectado ao banco. Equipes, jogadores e partidas carregados.\n");
    printf("CAMPEONATO DE E-SPORTS\n");
    while (continuar) {
        printf("\n1. Cadastrar equipe\n");
        printf("2. Listar equipes\n");
        printf("3. Registrar partida\n");
        printf("4. Exibir ranking\n");
        printf("5. Buscar equipe por ID\n");
        printf("6. Listar jogadores\n");
        printf("0. Sair\n");

        if (!ler_inteiro("Escolha uma opcao: ", &opcao)) {
            break;
        }
        switch (opcao) {
        case 1:
            continuar = cadastrar_equipe_menu(conexao, equipes,
                                              &quantidade_equipes);
            break;
        case 2:
            listar_equipes(equipes, quantidade_equipes);
            break;
        case 3:
            continuar = registrar_partida_menu(
                conexao, partidas, &quantidade_partidas, equipes,
                quantidade_equipes);
            break;
        case 4:
            continuar = exibir_ranking(equipes, quantidade_equipes, partidas,
                                       quantidade_partidas);
            break;
        case 5:
            buscar_equipe_menu(equipes, quantidade_equipes);
            break;
        case 6:
            listar_jogadores(jogadores, quantidade_jogadores);
            break;
        case 0:
            printf("Encerrando o campeonato.\n");
            continuar = 0;
            break;
        default:
            printf("Opcao invalida.\n");
            break;
        }
    }

    banco_desconectar(conexao);
    return 0;
}
