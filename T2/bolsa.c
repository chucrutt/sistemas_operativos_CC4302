#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Variables globales
int mejor_precio = 0;
char nombreComprador[100];
char nombreVendedor[100];

char **compradorLocal = NULL;
int *estadoVendedor = NULL;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int vendo(int precio, char *vendedor, char *comprador) {
    pthread_mutex_lock(&m);

    int estado = 0; // 0 = esperando, 1 = vendido, -1 = perdió

    if (estadoVendedor == NULL) {
        // Soy el único vendedor
        strcpy(nombreVendedor, vendedor);
        mejor_precio = precio;
        estadoVendedor = &estado;
        compradorLocal = &comprador;

        while (estado == 0) {
            pthread_cond_wait(&cond, &m);
        }

    } else {
        // Ya hay un vendedor
        if (precio > mejor_precio) {
            // Mi precio es peor, fracaso inmediato
            pthread_mutex_unlock(&m);
            return 0;
        } else {
            // Desplazo al vendedor anterior
            *estadoVendedor = -1; // el anterior pierde
            // Me registro como el nuevo vendedor
            strcpy(nombreVendedor, vendedor);
            mejor_precio = precio;
            estadoVendedor = &estado;
            compradorLocal = &comprador;

            pthread_cond_broadcast(&cond);

            while (estado == 0) {
                pthread_cond_wait(&cond, &m);
            }
        }
    }

    int resultado = estado;

    pthread_mutex_unlock(&m);
    return (resultado == 1);
}

int compro(char *comprador, char *vendedor) {
    pthread_mutex_lock(&m);

    if (estadoVendedor == NULL) {
        pthread_mutex_unlock(&m);
        return 0; // no hay vendedor
    }

    int precio = mejor_precio;

    // Copiar salidas
    strcpy(vendedor, nombreVendedor);
    strcpy(nombreComprador, comprador);

    // Notificar al vendedor que tuvo éxito
    *estadoVendedor = 1;
    estadoVendedor = NULL;

    strcpy(*compradorLocal, nombreComprador);
    compradorLocal = NULL;

    pthread_cond_broadcast(&cond);

    pthread_mutex_unlock(&m);
    return precio;
}
