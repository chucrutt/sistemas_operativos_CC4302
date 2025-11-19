#include <stdio.h>
#include <string.h>
#include "pss.h"

int main() {
    printf("=== EJEMPLO PRIORITY QUEUE ===\n");
    
    // Crear cola de prioridad
    PriQueue *pq = makePriQueue();
    
    // Insertar tareas con diferentes prioridades
    // Prioridad más alta = mayor valor numérico
    priPut(pq, "Tarea Crítica", 10.0);
    priPut(pq, "Tarea Normal", 5.0);
    priPut(pq, "Tarea Urgente", 8.0);
    priPut(pq, "Tarea Baja", 2.0);
    priPut(pq, "Tarea Muy Urgente", 9.5);
    
    printf("Insertadas 5 tareas con diferentes prioridades\n");
    printf("Longitud de la cola: %d\n", priLength(pq));
    printf("Mejor prioridad actual: %.1f\n", priBest(pq));
    printf("Peek (sin extraer): %s\n\n", (char*)priPeek(pq));
    
    // Extraer elementos por orden de prioridad
    printf("Extrayendo por orden de prioridad:\n");
    while (!emptyPriQueue(pq)) {
        double prioridad = priBest(pq);
        char *tarea = (char*)priGet(pq);
        printf("Extraído: %-20s (prioridad: %.1f, restantes: %d)\n", 
               tarea, prioridad, priLength(pq));
    }
    
    // Ejemplo de eliminación específica
    printf("\n=== EJEMPLO DE ELIMINACIÓN ESPECÍFICA ===\n");
    
    priPut(pq, "Tarea 1", 5.0);
    priPut(pq, "Tarea 2", 8.0);
    priPut(pq, "Tarea 3", 3.0);
    priPut(pq, "Tarea 4", 7.0);
    
    printf("Antes de eliminar 'Tarea 2': %d elementos\n", priLength(pq));
    
    if (priDel(pq, "Tarea 2") == 0) {
        printf("'Tarea 2' eliminada exitosamente\n");
    }
    
    printf("Después de eliminar: %d elementos\n", priLength(pq));
    printf("Elementos restantes:\n");
    while (!emptyPriQueue(pq)) {
        printf("- %s (prioridad: %.1f)\n", (char*)priPeek(pq), priBest(pq));
        priGet(pq);
    }
    
    destroyPriQueue(pq);
    return 0;
}