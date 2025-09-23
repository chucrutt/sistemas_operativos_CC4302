#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define N 10 // número de estacionamientos
#define MAX_WAITING 100 // máximo de solicitudes en espera

typedef struct {
    int k; // tamaño requerido
    int index; // índice en la cola
    pthread_cond_t cond; // condición para este hilo
    bool ready; // si ya puede reservar
    int result; // resultado de la reserva
} Solicitud;

bool estacionamientos[N];
pthread_mutex_t mutex;
Solicitud* cola[MAX_WAITING];
int front = 0, rear = 0, count = 0;

void initReservar() {
    for (int i = 0; i < N; i++) estacionamientos[i] = false;
    pthread_mutex_init(&mutex, NULL);
    front = rear = count = 0;
}

void cleanReservar() {
    pthread_mutex_destroy(&mutex);
    // No es necesario destruir condicionales individuales aquí
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

    // Crear solicitud y agregarla a la cola
    Solicitud solicitud;
    solicitud.k = k;
    solicitud.ready = false;
    solicitud.result = -1;
    pthread_cond_init(&solicitud.cond, NULL);

    cola[rear] = &solicitud;
    solicitud.index = rear;
    rear = (rear + 1) % MAX_WAITING;
    count++;

    // Esperar hasta que sea el primero y haya espacio
    while (true) {
        // Si es el primero en la cola y hay espacio
        if (cola[front] == &solicitud && (solicitud.result = buscar_libres(k)) != -1) {
            // Reservar
            for (int i = 0; i < k; i++) {
                estacionamientos[solicitud.result + i] = true;
            }
            // Sacar de la cola
            front = (front + 1) % MAX_WAITING;
            count--;
            solicitud.ready = true;
            pthread_cond_signal(&solicitud.cond);
            break;
        } else {
            // Esperar en su propia condición
            pthread_cond_wait(&solicitud.cond, &mutex);
        }
    }

    int res = solicitud.result;
    pthread_cond_destroy(&solicitud.cond);

    // Despertar al siguiente en la cola si corresponde
    if (count > 0) {
        Solicitud* siguiente = cola[front];
        pthread_cond_signal(&siguiente->cond);
    }

    pthread_mutex_unlock(&mutex);
    return res;
}

// Libera k estacionamientos desde el índice e
void liberar(int e, int k) {
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < k; i++) {
        estacionamientos[e + i] = false;
    }
    // Despertar al primero en la cola si corresponde
    if (count > 0) {
        Solicitud* siguiente = cola[front];
        pthread_cond_signal(&siguiente->cond);
    }
    pthread_mutex_unlock(&mutex);
}