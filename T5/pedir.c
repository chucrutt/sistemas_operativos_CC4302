#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "nthread-impl.h"
#include "pss.h"
#include "pedir.h"

// Variables globales
static NthQueue *queue0;  // Cola para threads de categoría 0
static NthQueue *queue1;  // Cola para threads de categoría 1
static int resourceFree = 1;    // 1: libre, 0: ocupado
static nThread resourceOwner = NULL; // Thread que posee el recurso
static int lastCategory = -1;   // Última categoría que tuvo el recurso

// Función de timeout que maneja la eliminación de la cola
static void timeoutWakeUp(nThread th) {
    // Esta función se ejecuta antes de que setReady sea llamada
    int cat = (int)(intptr_t)th->ptr;
    
    if (cat == 0) {
        nth_delQueue(queue0, th);
    } else {
        nth_delQueue(queue1, th);
    }
}

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
        return 1;
    }
    
    // El recurso está ocupado, debemos esperar
    // Guardar la categoría en el campo ptr
    thisTh->ptr = (void *)(intptr_t)cat;
    
    // Agregar a la cola correspondiente
    if (cat == 0) {
        nth_putBack(queue0, thisTh);
    } else {
        nth_putBack(queue1, thisTh);
    }
    
    // Configurar el estado y timeout según corresponda
    if (timeout > 0) {
        suspend(WAIT_REQUEST_TIMEOUT);
        // Convertir milisegundos a nanosegundos
        nth_programTimer(timeout * 1000000LL, timeoutWakeUp);
    } else {
        suspend(WAIT_REQUEST);
    }
    
    schedule();
    
    // Al despertar, verificar el estado para determinar si fue timeout o éxito
    int result;
    if (resourceOwner == thisTh) {
        result = 1;
    } else {
        result = 0;  // Timeout
        // El timeout handler ya removió el thread de la cola
    }
    
    // Limpiar la información
    thisTh->ptr = NULL;
    
    END_CRITICAL
    return result;
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
        // Si el thread tenía timeout programado, cancelarlo
        if (nextTh->status == WAIT_REQUEST_TIMEOUT) {
            nth_cancelThread(nextTh);
        }
        
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
