#include <stdio.h>
#include <stdlib.h>
#include <nthread-impl.h>
#include "pss.h"
#include "pedir.h"

// Variables globales
static NthQueue *queue0;  // Cola para threads de categoría 0
static NthQueue *queue1;  // Cola para threads de categoría 1
static int resourceFree = 1;    // 1: libre, 0: ocupado
static nThread resourceOwner = NULL; // Thread que posee el recurso
static int lastCategory = -1;   // Última categoría que tuvo el recurso

void nth_iniciar() {
    queue0 = nth_makeQueue();
    queue1 = nth_makeQueue();
    resourceFree = 1;
    resourceOwner = NULL;
    lastCategory = -1;
}

void nth_terminar() {
    nth_destroyQueue(queue0);
    nth_destroyQueue(queue1);
}

int nPedir(int cat, int timeout) {
    START_CRITICAL
    
    nThread thisTh = nSelf();
    
    // Si el recurso está libre, lo asignamos inmediatamente
    if (resourceFree) {
        resourceFree = 0;
        resourceOwner = thisTh;
        lastCategory = cat;
        
        END_CRITICAL
        return 0;
    }
    
    // El recurso está ocupado, debemos esperar
    // Agregar a la cola correspondiente
    if (cat == 0) {
        nth_putBack(queue0, thisTh);
    } else {
        nth_putBack(queue1, thisTh);
    }
    
    suspend(WAIT_REQUEST);
    schedule();
    
    END_CRITICAL
    return 0;
}

void nDevolver() {
    START_CRITICAL
    
    nThread thisTh = nSelf();
    
    if (resourceOwner != thisTh) {
        END_CRITICAL
        return;
    }
    
    // Política alternada: primero intentar asignar a la categoría opuesta
    int oppositeCategory = (lastCategory == 0) ? 1 : 0;
    NthQueue *oppositeQueue = (oppositeCategory == 0) ? queue0 : queue1;
    NthQueue *sameQueue = (lastCategory == 0) ? queue0 : queue1;
    
    nThread nextTh = NULL;
    int nextCategory = -1;
    
    // Primero buscar en la cola de la categoría opuesta
    if (!nth_emptyQueue(oppositeQueue)) {
        nextTh = nth_getFront(oppositeQueue);
        nextCategory = oppositeCategory;
    }
    // Si no hay en la opuesta, buscar en la misma categoría
    else if (!nth_emptyQueue(sameQueue)) {
        nextTh = nth_getFront(sameQueue);
        nextCategory = lastCategory;
    }
    
    // Si hay un thread esperando, asignarle el recurso
    if (nextTh != NULL) {
        resourceOwner = nextTh;
        lastCategory = nextCategory;
        setReady(nextTh);
    }
    // Si no hay nadie esperando, liberar el recurso
    else {
        resourceFree = 1;
        resourceOwner = NULL;
        // lastCategory se mantiene para futuras decisiones
    }
    
    END_CRITICAL
}

