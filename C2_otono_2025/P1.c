#include <pthread.h>
#include "pss.h"

#define N 10
int notebooks[N]; // 0: disponible, 1: ocupado
Queue *q;
pthread_mutex_t m;


typedef struct {
    int ntbk;
    int ready;
    pthread_cond_t cond;
} Solicitud;


void init() {
    q = makeQueue();
    pthread_mutex_init(&m, NULL);
}

int solNtbk() {
    pthread_mutex_lock(&m);
    int disponible = -1; // Identificador de notebook disponible, -1: no disponible

    for (int i = 0; i < N; i++) {
        if (!notebooks[i]){
            disponible = i;
            notebooks[i] = 1;
            break;
        }
    }

    // Si no hay notebooks disponibles
    if (disponible == -1) {
        // Crear la solicitud y agregar a cola de espera
        Solicitud *solicitud;
        solicitud -> ready = 0;
        pthread_cond_init(&solicitud -> cond, NULL);
        put(q, solicitud);

        while(solicitud -> ready == 0) {
            pthread_cond_wait(&solicitud -> cond, &m);
        }

        disponible = solicitud -> ntbk;

        pthread_cond_destroy(&solicitud -> cond);
        free(solicitud);
        pthread_mutex_unlock(&m);

        return disponible;
    }

    pthread_mutex_unlock(&m);
    return disponible;
}

void devNtbk(int i) {
    pthread_mutex_lock(&m);

    if (!emptyQueue(q)) {
        Solicitud *siguiente = peek(q);
        siguiente -> ntbk = i;
        siguiente -> ready = 1;
        pthread_cond_signal(&siguiente -> cond);
    } else {
        notebooks[i] = 0;
    }
    
    pthread_mutex_unlock(&m);
    return;
}

/* b) ¿Cómo se garantiza la exclusión mutua al acceder a la cola de procesos ready en un núcleo de sistema operativo para un computador single-core? ¿Y si el computador es multi-core?

    - En un sistema monocore, la exclusión mutua dentro del núcleo se garantiza principalmente bloqueando la única fuente de concurrencia: las interrupciones. Basta con bloquear las señales que pueden interrumpir la ejecución de un thread y causar un cambio de contexto mientras se manipula la memoria compartida.

    - Para un sistema multicore, multiples núcleos pueden acceder simultaneamente a memoria compartida por lo que se requiere un método de sincronización más robusto como spin-locks (uno por cada estructura crítica) o un mutex compartido.
*/
/* c) Dé un ejemplo en el que solo utilizando shortest job first (SJF) se genere hambruna.
    - SJF consiste en dar prioridad a tareas más cortas, lo que permite minimizar el tiempo de espera promedio. Sin embargo, puede ocurrir que luego de una tarea muy pesada (baja prioridad) le siguen un número indeterminado de tareas más cortas (de mayor prioridad) éste nunca sea ejecutado (hambruna).
*/