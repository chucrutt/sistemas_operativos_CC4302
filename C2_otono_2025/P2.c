#include <nthread-impl.h>
#include "pss.h"

#define N 10
#define WAIT_NTBK  100 // Estado para threads esperando notebook
#define DISPONIBLE 0
#define OCUPADO 1

// Variables globales del sistema
int notebooks_status[N];     // 0: disponible, 1: ocupado
int notebooks_id[N];         // notebooks_id[i] = i (siempre)
Queue *waiting_queue;        // Cola FIFO para threads esperando
PriQueue *available_queue;   // Cola de prioridad para notebooks disponibles
int global_counter;          // Contador para aging (mayor = más reciente)
int notebook_assigned;   // ID del notebook asignado (-1 si no hay)

void init() {
    // Inicializar colas
    waiting_queue = makeQueue();
    available_queue = makePriQueue();
    global_counter = 0;
    
    // Inicializar arreglo de IDs (notebooks_id[i] = i)
    for (int i = 0; i < N; i++) {
        notebooks_id[i] = i;
        notebooks_status[i] = DISPONIBLE;
        
        // Agregar todos los notebooks a la cola de prioridad
        // Prioridad = 0 para todos inicialmente (mismo tiempo sin usar)
        priPut(available_queue, &notebooks_id[i], global_counter);
    }
}

int solNtbk() {
    START_CRITICAL;
    
    // Verificar si hay notebooks disponibles en la cola de prioridad
    if (!emptyPriQueue(available_queue)) {
        // Obtener el notebook que lleva más tiempo sin usar
        int *notebook_ptr = (int *)priGet(available_queue);
        int notebook_id = *notebook_ptr;
        
        // Marcar como ocupado
        notebooks_status[notebook_id] = OCUPADO;
        
        END_CRITICAL;
        return notebook_id;
    }

    // No hay notebooks disponibles - thread debe esperar
    nThread current = nSelf();
    notebook_assigned = -1;
    current->ptr = notebook_assigned;
    
    // Agregar thread a cola de espera (FIFO)
    put(waiting_queue, current);
    
    // Cambiar estado y suspender
    current->status = WAIT_NTBK;
    suspend(WAIT_NTBK);
    schedule();

    // Al despertar, el notebook ya está asignado en thread_data
    int assigned_notebook = notebook_assigned;
    
    END_CRITICAL;
    return assigned_notebook;
}

void devNtbk(int notebook_id) {
    START_CRITICAL;

    // Liberar notebook y actualizar prioridad
    notebooks_status[notebook_id] = DISPONIBLE;
    global_counter--;
    priPut(available_queue, notebook_id, global_counter);

    // Verificar si hay threads esperando
    if (!emptyQueue(waiting_queue)) {
        // Obtener primer thread en espera (FIFO)
        nThread waiting_thread = get(waiting_queue);
        
        // Asignar notebook al thread
        int nextNtbk = priGet(available_queue);
        *(waiting_thread -> ptr) = nextNtbk;
        notebooks_status[nextNtbk] = OCUPADO;
        
        // Despertar al thread
        setReady(waiting_thread);
        schedule();
    }
    END_CRITICAL;
}