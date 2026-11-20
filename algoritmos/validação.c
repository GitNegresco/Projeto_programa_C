#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "validacao.h"

int ler_inteiro(const char *mensagem, int minimo, int maximo) {
    char entrada[100];
    char *fim;
    long valor;

    while (1) {
        printf("%s", mensagem);

        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            return minimo;
        }

        errno = 0;
        valor = strtol(entrada, &fim, 10);

        if (fim == entrada || errno == ERANGE) {
            printf("Digite um numero inteiro valido.\n");
            continue;
        }

        while (isspace((unsigned char)*fim)) fim++;

        if (*fim != '\0') {
            printf("Entrada invalida.\n");
            continue;
        }

        if (valor < minimo || valor > maximo) {
            printf("Digite um valor entre %d e %d.\n", minimo, maximo);
            continue;
        }

        return (int)valor;
    }
}

void ler_texto(const char *mensagem, char texto[], int tamanho) {
    if (texto == NULL || tamanho <= 0) return;

    while (1) {
        printf("%s", mensagem);

        if (fgets(texto, tamanho, stdin) == NULL) {
            texto[0] = '\0';
            return;
        }

        texto[strcspn(texto, "\n")] = '\0';

        int vazio = 1;

        for (int i = 0; texto[i] != '\0'; i++) {
            if (!isspace((unsigned char)texto[i])) {
                vazio = 0;
                break;
            }
        }

        if (vazio) {
            printf("O campo nao pode ficar vazio.\n");
            continue;
        }

        return;
    }
}
