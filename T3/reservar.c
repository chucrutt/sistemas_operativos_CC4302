#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pss.h"

#define N 10 // número de estacionamientos
#define MAX_WAITING 100 // máximo de solicitudes en espera

typedef struct {
    int k; // tamaño requerido
    pthread_cond_t cond; // condición para este hilo
    bool ready; // si ya puede reservar
    int result; // resultado de la reserva
} Solicitud;

bool estacionamientos[N];
pthread_mutex_t mutex;
Queue *q;

void initReservar() {
    for (int i = 0; i < N; i++) estacionamientos[i] = false;
    pthread_mutex_init(&mutex, NULL);
    q = makeQueue();
}

void cleanReservar() {
    pthread_mutex_destroy(&mutex);
}

// Busca k estacionamientos contiguos libres, retorna el índice inicial o -1 si no hay
int buscar_libres(int k) {
    for (int i = 0; i <= N - k; i++) {
        bool libres = true;
        for (int j = 0; j < k; j++) {
            if (estacionamientos[i + j]) {
                libres = false;
                break;
            }
        }
        if (libres) return i;
    }
    return -1;
}

// Reserva k estacionamientos contiguos, retorna el índice inicial
int reservar(int k) {
    pthread_mutex_lock(&mutex);
    
    // Buscar espacio disponible si no hay nadie esperando
    int libres = buscar_libres(k);
    
    // Si hay gente esperando o no hay espacio, debo esperar en la cola (FIFO)
    if (!emptyQueue(q) || libres == -1) {
        // Crear solicitud y agregarla a la cola
        Solicitud *solicitud = malloc(sizeof(Solicitud));
        solicitud->k = k;
        solicitud->ready = false;
        solicitud->result = -1;
        pthread_cond_init(&solicitud->cond, NULL);
        put(q, solicitud);
        
        // Esperar hasta que esté lista
        while (!solicitud->ready) {
            pthread_cond_wait(&solicitud->cond, &mutex);
        }
        
        // Obtener el resultado (ya están marcados como ocupados por liberar)
        libres = solicitud->result;
        pthread_cond_destroy(&solicitud->cond);
        free(solicitud);

        pthread_mutex_unlock(&mutex);
        return libres;
    }

    // Marcar los estacionamientos como ocupados (caso donde hay espacio inmediato)
    for (int i = 0; i < k; i++) {
        estacionamientos[libres + i] = true;
    }

    pthread_mutex_unlock(&mutex);
    return libres;
}

// Libera k estacionamientos desde el índice e
void liberar(int e, int k) {
    pthread_mutex_lock(&mutex);
    
    // Liberar los estacionamientos
    for (int i = 0; i < k; i++) {
        estacionamientos[e + i] = false;
    }
    
    // Intentar satisfacer solicitudes en orden FIFO
    while (!emptyQueue(q)) {
        Solicitud *primera = peek(q);
        int pos = buscar_libres(primera->k);
        
        if (pos != -1) {
            // Puede satisfacerse la primera solicitud
            primera = get(q); // Remover de la cola
            
            // Marcar inmediatamente como ocupados para evitar condiciones de carrera
            for (int i = 0; i < primera->k; i++) {
                estacionamientos[pos + i] = true;
            }
            
            primera->result = pos;
            primera->ready = true;
            pthread_cond_signal(&primera->cond);
            // Continuar para ver si hay más solicitudes que se pueden satisfacer
        } else {
            // No se puede satisfacer la primera, no hay punto en revisar las demás
            break;
        }
    }
    
    pthread_mutex_unlock(&mutex);
}