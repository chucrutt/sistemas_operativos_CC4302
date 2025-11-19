#include <stdio.h>
#include <string.h>
#include "pss.h"

typedef struct {
    char nombre[20];
    int tiempo_llegada;
    int prioridad;
} Proceso;

int main() {
    printf("=== COMPARACIÓN: SCHEDULING FCFS vs PRIORITY ===\n\n");
    
    // Procesos de ejemplo
    Proceso procesos[] = {
        {"ProcesoA", 0, 3},
        {"ProcesoB", 1, 8},  // Alta prioridad
        {"ProcesoC", 2, 1},  // Baja prioridad
        {"ProcesoD", 3, 6},
        {"ProcesoE", 4, 9}   // Muy alta prioridad
    };
    
    int num_procesos = 5;
    
    // === SCHEDULING FCFS (First Come First Served) ===
    printf("1. FCFS - Cola normal (orden de llegada):\n");
    Queue *fcfs = makeQueue();
    
    for (int i = 0; i < num_procesos; i++) {
        put(fcfs, &procesos[i]);
        printf("   Llegó: %s (prioridad %d)\n", 
               procesos[i].nombre, procesos[i].prioridad);
    }
    
    printf("   Orden de ejecución FCFS:\n");
    int orden_fcfs = 1;
    while (!emptyQueue(fcfs)) {
        Proceso *p = (Proceso*)get(fcfs);
        printf("   %d. %s (prioridad %d)\n", orden_fcfs++, p->nombre, p->prioridad);
    }
    
    // === PRIORITY SCHEDULING ===
    printf("\n2. PRIORITY - Cola de prioridad (mayor prioridad primero):\n");
    PriQueue *priority = makePriQueue();
    
    for (int i = 0; i < num_procesos; i++) {
        priPut(priority, &procesos[i], procesos[i].prioridad);
        printf("   Llegó: %s (prioridad %d)\n", 
               procesos[i].nombre, procesos[i].prioridad);
    }
    
    printf("   Orden de ejecución PRIORITY:\n");
    int orden_pri = 1;
    while (!emptyPriQueue(priority)) {
        Proceso *p = (Proceso*)priGet(priority);
        printf("   %d. %s (prioridad %d)\n", orden_pri++, p->nombre, p->prioridad);
    }
    
    printf("\n=== ANÁLISIS ===\n");
    printf("- FCFS: Justo pero puede causar starvation de procesos importantes\n");
    printf("- PRIORITY: Procesos importantes se ejecutan primero, pero puede causar\n");
    printf("  starvation de procesos de baja prioridad\n");
    
    destroyQueue(fcfs);
    destroyPriQueue(priority);
    return 0;
}