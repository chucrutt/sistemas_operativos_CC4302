#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pss.h"
#include "spinlocks.h"
#include "disco.h"

typedef struct WaitEntry {
  char *name;         // nombre de la persona
  char *pareja;       // nombre de la pareja asignada
  volatile int lock;  // spin-lock personal para esperar señal
} WaitEntry;

static Queue *qdamas = NULL;
static Queue *qvarones = NULL;
static int glock = OPEN;

void discoInit() {
  qdamas = makeQueue();
  qvarones = makeQueue();
  glock = OPEN;
}

void discoDestroy() {
  // vaciar y liberar cualquier entry que quede (por seguridad)
  while (!emptyQueue(qdamas))
    free(get(qdamas));
  while (!emptyQueue(qvarones))
    free(get(qvarones));
  destroyQueue(qdamas);
  destroyQueue(qvarones);
  qdamas = qvarones = NULL;
}

char *varon(char *nom) {
  WaitEntry *me = NULL;

  spinLock(&glock);
  if (!emptyQueue(qdamas)) {
    // hay una dama esperando: emparejar con la que llegó primero
    WaitEntry *opp = (WaitEntry *)get(qdamas);
    char *res = opp->name;
    opp->pareja = nom; // comunicar nuestro nombre a la dama
    spinUnlock(&glock);
    // despertar a la dama
    spinUnlock((int *)&opp->lock);
    return res;
  }
  else {
    // no hay damas: encolarse y esperar
    me = malloc(sizeof(WaitEntry));
    me->name = nom;
    me->pareja = NULL;
    me->lock = CLOSED; // permanecer cerrado hasta que nos despierten
    put(qvarones, me);
    spinUnlock(&glock);

    // esperar señal de emparejamiento
    spinLock(&me->lock);
    char *res = me->pareja;
    free(me);
    return res;
  }
}

char *dama(char *nom) {
  WaitEntry *me = NULL;

  spinLock(&glock);
  if (!emptyQueue(qvarones)) {
    // hay varon esperando: emparejar con el que llegó primero
    WaitEntry *opp = (WaitEntry *)get(qvarones);
    char *res = opp->name;
    opp->pareja = nom; // comunicar nuestro nombre al varon
    spinUnlock(&glock);
    // despertar al varon
    spinUnlock((int *)&opp->lock);
    return res;
  }
  else {
    // no hay varones: encolarse y esperar
    me = malloc(sizeof(WaitEntry));
    me->name = nom;
    me->pareja = NULL;
    me->lock = CLOSED;
    put(qdamas, me);
    spinUnlock(&glock);

    // esperar señal de emparejamiento
    spinLock(&me->lock);
    char *res = me->pareja;
    free(me);
    return res;
  }
}
