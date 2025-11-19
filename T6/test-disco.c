#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "disco.h"
#include "spinlocks.h"

// Parametro para el test simple
#define TICK 500000

// Parametro para el test de orden de llegada
#define NESPERAN 5
#define PAUSA 200000

// Parametros del test de robustez
#ifdef OPT
#define NPAREJAS 20
#define NBAILES 100
#else
#define NPAREJAS 10
#define NBAILES 100
#endif

void fatalError( char *procname, char *format, ... ) {
  va_list ap;

  va_start(ap, format);
  fprintf(stderr, "%s: ", procname);
  vfprintf(stderr, format, ap);
  va_end(ap);

  exit(1);
}

void microsleep(useconds_t usec) {
  int rc= usleep(usec);
  if (rc!=0)
    fatalError("microsleep", "usleep ended with error\n");
}

#if 1

// Test Simple

typedef struct {
  pthread_t t;
  char *nom, **ppareja;
} Persona;

static void *damaFun(void *ptr) {
  Persona *ppers= ptr;
  char *nom= ppers->nom, **ppareja= ppers->ppareja;
  printf("%s espera pareja\n", nom);
  *ppareja= dama(nom);
  if (*ppareja==NULL)
    fatalError("dama", "La pareja de %s es NULL\n", nom);
  printf("La pareja de %s es %s\n", nom, *ppareja);
  return NULL;
}

static void *varonFun(void *ptr) {
  Persona *ppers= ptr;
  char *nom= ppers->nom, **ppareja= ppers->ppareja;
  printf("%s espera pareja\n", nom);
  *ppareja= varon(nom);
  if (*ppareja==NULL)
    fatalError("varon", "La pareja de %s es NULL\n", nom);
  printf("La pareja de %s es %s\n", nom, *ppareja);
  return NULL;
}

Persona *startPersona(Persona *ppers, void *(*fun)(void *),
                      char *nom, char **ppareja) {
  ppers->nom= nom;
  ppers->ppareja= ppareja;
  if (pthread_create(&ppers->t, NULL, fun, ppers)!=0) {
    perror("pthread_create");
    exit(1);
  }
  return ppers;
}

void joinPersona(Persona *ppers) {
  if (pthread_join(ppers->t, NULL)!=0) {
    perror("pthread_join");
    exit(1);
  }
}

static void verificar_pareja(char *nom_dama, char *nom_varon,
                      char *pareja_dama, char *pareja_varon) {
  if (strcmp(pareja_dama, nom_varon)!=0)
    fatalError("verificar_pareja", "La pareja de %s debio ser %s, "
                "pero es %s\n", nom_dama, nom_varon, pareja_dama);
  if (strcmp(pareja_varon, nom_dama)!=0)
    fatalError("verificar_pareja", "La pareja de %s debio ser %s, "
                "pero es %s\n", nom_varon, nom_dama, pareja_varon);
}

static void testSimple() {
  printf("Test: una sola pareja, adan y eva\n");
  char *pareja_eva= NULL, *pareja_adan= NULL;
  Persona eva, adan, ana, sara, pedro, juan, diego, alba;
  startPersona(&eva, damaFun, "eva", &pareja_eva);
  microsleep(TICK);
  if (pareja_eva!=NULL)
    fatalError("testSimple", "eva tenia que esperar a adan\n");
  startPersona(&adan, varonFun, "adan", &pareja_adan);
  joinPersona(&eva);
  joinPersona(&adan);
  verificar_pareja("eva", "adan", pareja_eva, pareja_adan);
  printf("Aprobado\n");

  printf("Test: el ejemplo del enunciado\n");
  char *pareja_ana= NULL, *pareja_sara= NULL, *pareja_alba= NULL;
  char *pareja_pedro= NULL, *pareja_juan= NULL, *pareja_diego= NULL;
  startPersona(&ana, damaFun, "ana", &pareja_ana);
  microsleep(TICK);
  startPersona(&sara, damaFun, "sara", &pareja_sara);
  microsleep(TICK);
  startPersona(&pedro, varonFun, "pedro", &pareja_pedro);
  joinPersona(&ana);
  joinPersona(&pedro);
  verificar_pareja("ana", "pedro", pareja_ana, pareja_pedro);
  microsleep(TICK);
  startPersona(&juan, varonFun, "juan", &pareja_juan);
  joinPersona(&sara);
  joinPersona(&juan);
  verificar_pareja("sara", "juan", pareja_sara, pareja_juan);
  microsleep(TICK);
  startPersona(&diego, varonFun, "diego", &pareja_diego);
  microsleep(TICK);
  startPersona(&alba, damaFun, "alba", &pareja_alba);
  joinPersona(&alba);
  joinPersona(&diego);
  verificar_pareja("alba", "diego", pareja_alba, pareja_diego);
  printf("Aprobado\n");
}

// Test de orden de llegada

static Persona wtasks[NESPERAN];
static char *wnoms[NESPERAN];
static char *wparejas[NESPERAN];

static void *wait_dama(void *ptr) {
  Persona *ppers= ptr;
  char *nom= ppers->nom, **ppareja= ppers->ppareja;
  *ppareja= dama(nom);
  return 0;
}

void *wait_varon(void *ptr) {
  Persona *ppers= ptr;
  char *nom= ppers->nom, **ppareja= ppers->ppareja;
  *ppareja= varon(nom);
  return 0;
}

static void testOrden() {
  printf("Test: Orden de llegada de los varones\n");
  for (int i= 0; i<NESPERAN; i++) {
    wnoms[i]= malloc(16);
    sprintf(wnoms[i], "%d", i);
    startPersona(&wtasks[i], wait_varon, wnoms[i], &wparejas[i]);
    printf("thread %d created\n", i);
    microsleep(PAUSA); // Esto asegura que la tarea ya invoco varon
  }
  for (int i= 0; i<NESPERAN; i++) {
    char *pareja= dama("eva"); // Todos bailan con eva
    if (i != atoi(pareja))
      fatalError("testOrden", "Varon %s no respeta orden de llegada\n",
                  pareja);
    joinPersona(&wtasks[i]);
    printf("thread %d joined\n", i);
  }
  printf("Aprobado\n");
  printf("Test: Orden de llegada de las damas\n");
  for (int i= 0; i<NESPERAN; i++) {
    startPersona(&wtasks[i], wait_dama, wnoms[i], &wparejas[i]);
    printf("thread %d created\n", i);
    microsleep(PAUSA); // Esto asegura que la tarea ya invoco varon
  }
  for (int i= 0; i<NESPERAN; i++) {
    char *pareja= varon("adan"); // Todas bailan con adan
    if (i != atoi(pareja))
      fatalError("testOrden", "Dama %s no respeta orden de llegada\n",
                  pareja);
    joinPersona(&wtasks[i]);
    printf("thread %d joined\n", i);
  }
  for (int i= 0; i<NESPERAN; i++) {
    free(wnoms[i]);
  }
  printf("Aprobado\n");
}

#endif

// Test de robustez

static sem_t anotadas[NPAREJAS], verificadas[NPAREJAS];
static int pareja_damas[NPAREJAS];
static pthread_t tasks_damas[NPAREJAS], tasks_varones[NPAREJAS];
static char *noms[NPAREJAS];

static void *loop_dama(void *ptr) {
  char *nom= ptr;
  int iter= NBAILES;
  int id= atoi(nom);
  while (iter--) {
    char *pareja= dama(nom);
    int id_pareja= atoi(pareja);
    pareja_damas[id]= id_pareja;
    sem_post(&anotadas[id]);
    sem_wait(&verificadas[id]);
    pareja_damas[id]= -1;
  }
  return 0;
}

static void *loop_varon(void *ptr) {
  char *nom= ptr;
  int id= atoi(nom);
  int iter= NBAILES;
  while (iter--) {
    char *pareja= varon(nom);
    int id_pareja= atoi(pareja);
    sem_wait(&anotadas[id_pareja]);
    if (pareja_damas[id_pareja]!=id)
      fatalError("loop_varon", "Varon %s dice que su pareja es dama %s, "
                                "pero dama %s dice que su pareja es varon %d\n",
                  nom, pareja, pareja, pareja_damas[id_pareja]); 
    sem_post(&verificadas[id_pareja]);
  }
  return 0;
}

static void testRobustez() {
  printf("Test: robustez\n");
  for (int i= 0; i<NPAREJAS; i++) {
    sem_init(&anotadas[i], 0, 0);
    sem_init(&verificadas[i], 0, 0);
    pareja_damas[i]= -1;
    noms[i]=malloc(16);
    sprintf(noms[i], "%d", i);
    int check= atoi(noms[i]);
    if (check!=i)
      fatalError("testRobustez", "check");
    if (pthread_create(&tasks_damas[i], NULL, loop_dama, noms[i])!=0) {
      perror("pthread_create");
      exit(0);
    }
    if (pthread_create(&tasks_varones[i], NULL, loop_varon, noms[i])!=0) {
      perror("pthread_create");
      exit(0);
    }
  }
  for (int i= 0; i<NPAREJAS; i++) {
    pthread_join(tasks_damas[i], NULL);
    pthread_join(tasks_varones[i], NULL);
  }
  for (int i= 0; i<NPAREJAS; i++) {
    free(noms[i]);
    sem_destroy(&anotadas[i]);
    sem_destroy(&verificadas[i]);
  }
  printf("Aprobado\n");
}

int main(int argc, char *argv[]) {
  int bwtop= argc <=1 ? 2 : 1;
  discoInit();
  for (int bw= 0; bw<bwtop; bw++) {

    if(bw==0)
      printf("Prueba con spinlocks implementados con mutex sin busywaiting\n");
    else { 
      printf("\n");
      printf("===============================================\n");
      printf("Prueba con spinlocks verdaderos con busywaiting\n");
      printf("===============================================\n\n");
      setBusyWaiting(bw);
    }

    testSimple();
    testOrden();
    testRobustez();
  }
  discoDestroy();
  printf("Felicitaciones: aprobo todos los tests\n");
  return 0;
}
