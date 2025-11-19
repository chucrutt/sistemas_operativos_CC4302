#include <stdio.h>
#include <string.h>
#include "pss.h"

int main() {
    // Crear una cola
    Queue *q = makeQueue();
    
    printf("=== EJEMPLO QUEUE (FIFO) ===\n");
    
    // Insertar elementos (strings)
    char *tareas[] = {"Tarea A", "Tarea B", "Tarea C", "Tarea D"};
    
    for (int i = 0; i < 4; i++) {
        put(q, tareas[i]);
        printf("Insertado: %s (longitud: %d)\n", tareas[i], queueLength(q));
    }
    
    printf("\nPeek (sin extraer): %s\n", (char*)peek(q));
    printf("Longitud después de peek: %d\n\n", queueLength(q));
    
    // Extraer elementos en orden FIFO
    printf("Extrayendo elementos:\n");
    while (!emptyQueue(q)) {
        char *tarea = (char*)get(q);
        printf("Extraído: %s (longitud restante: %d)\n", tarea, queueLength(q));
    }
    
    destroyQueue(q);
    return 0;
}