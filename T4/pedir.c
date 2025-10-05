#include <stdio.h>
#include <nthread-impl.h>
#include "pss.h"
#include "pedir.h"


// Variables globales
int recurso_disponible;  // 1 si el recurso esta disponible, 0 si no
int categoria_actual;   // Categoria del thread que posee el recurso (-1 si no hay owner)
NthQueue cola_cat0; // Cola para threads de categoria 0
NthQueue cola_cat1; // Cola para threads de categoria 1

void nth_iniciar() {
    cola_cat0 = nth_makeQueue;
    cola_cat1 = nth_makeQueue;
    recurso_disponible = 1;
    categoria_actual = -1;
}

void nth_terminar() {
    nth_destroyQueue(&cola_cat0);
    nth_destroyQueue(&cola_cat1);
}

int nPedir(int cat, int timeout) {
    START_CRITICAL;
    
    // Verificar si el recurso esta disponible
    if (recurso_disponible) {
        // Asignar el recurso al thread actual
        recurso_disponible = 0;
        categoria_actual = cat;
        END_CRITICAL;
        return 1;
    }
    
    // El recurso no esta disponible, agregar el thread a la cola correspondiente
    pthread_t th = nSelf();
    if (cat == 0) {
        nth_putBack(&cola_cat0, th);
    } else {
        nth_putBack(&cola_cat1, th);
    }
    
    // Suspender el thread hasta que se le asigne el recurso
    suspend(WAIT_REQUEST);
    schedule();
    
    END_CRITICAL;
    return 1;
}

void nDevolver() {
    START_CRITICAL;
    
    // Liberar el recurso
    recurso_disponible = 1;
    int cat_anterior = categoria_actual;
    categoria_actual = -1;
    
    // Politica de asignacion alternada:
    // 1. Primero intentar asignar a la categoria opuesta
    // 2. Si no hay threads de categoria opuesta, asignar a la misma categoria
    // 3. Si no hay threads esperando, dejar el recurso disponible
    
    Queue *cola_opuesta, *cola_misma;
    if (cat_anterior == 0) {
        cola_opuesta = &cola_cat1;
        cola_misma = &cola_cat0;
    } else {
        cola_opuesta = &cola_cat0;
        cola_misma = &cola_cat1;
    }
    
    pthread_t th_siguiente = NULL;
    
    // Intentar asignar a categoria opuesta primero
    if (!nth_emptyQueue(cola_opuesta)) {
        th_siguiente = nth_getFront(cola_opuesta);
        categoria_actual = (cat_anterior == 0) ? 1 : 0;
    }
    // Si no hay threads de categoria opuesta, intentar con misma categoria
    else if (!nth_emptyQueue(cola_misma)) {
        th_siguiente = nth_getFront(cola_misma);
        categoria_actual = cat_anterior;
    }
    
    // Si encontramos un thread esperando, asignar el recurso y activarlo
    if (th_siguiente != NULL) {
        recurso_disponible = 0;
        setReady(th_siguiente);
        schedule();
    }
    
    END_CRITICAL;
}

