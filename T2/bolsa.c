#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Variables globales
int mejor_precio = 0;
char nombreComprador[100];
char nombreVendedor[100];
// Punteros a variables locales
char **compradorLocal = NULL;
int *estadoVendedor = NULL;

/* Hints:
    - No retornar variables globales pues su valor puede cambiar, es mejor guardar el resultado en una variable local.
    - No hay control sobre cuándo va a despertar un vendedor dormido por lo que el vendedor no debe cambiar variables globales, solo debe enterarse de su estado al despertar. 
    - Comprador es quien informa del estado al vendedor, consulta y cambia las variables globales usando punteros globales a las variables locales.
*/

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int vendo(int precio, char *vendedor, char *comprador) {
    pthread_mutex_lock(&m);

    int estado = 0; // 0 = esperando, 1 = vendido, -1 = perdió

    if (estadoVendedor == NULL) { // Soy el único vendedor
        // Registrar como el nuevo vendedor
        strcpy(nombreVendedor, vendedor);
        mejor_precio = precio;
        estadoVendedor = &estado;
        compradorLocal = &comprador;
        // Espero
        while (estado == 0) {
            pthread_cond_wait(&cond, &m);
        }

    } else { // Ya hay un vendedor
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
            // Despierto al antiguo vendedor
            pthread_cond_broadcast(&cond);
            // Espero
            while (estado == 0) {
                pthread_cond_wait(&cond, &m);
            }
        }
    }
    // Thread despierta y consulta su estado
    int resultado = estado;
    // Libera el mutex y retorna 0 (perdió) o 1 (ganó)
    pthread_mutex_unlock(&m);
    return (resultado == 1);
}

int compro(char *comprador, char *vendedor) {
    pthread_mutex_lock(&m);

    if (estadoVendedor == NULL) { // no hay vendedor
        pthread_mutex_unlock(&m);
        return 0;
    }

    int precio = mejor_precio;

    // Copiar salidas
    strcpy(vendedor, nombreVendedor);
    strcpy(nombreComprador, comprador);

    // Notificar al vendedor que tuvo éxito
    *estadoVendedor = 1;
    estadoVendedor = NULL;

    // Notificar al vendedor quien compra
    strcpy(*compradorLocal, nombreComprador);
    compradorLocal = NULL;

    // Despertar al vendedor
    pthread_cond_broadcast(&cond);

    // Liberar el mutex y retornar precio de venta (local)
    pthread_mutex_unlock(&m);
    return precio;
}
