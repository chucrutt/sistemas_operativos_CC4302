#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Declare aca sus variables globales
// ...

typedef struct {
  int precio;
  char comprador[100];
  char vendedor[100];
} Venta;
bool vendo = false;
bool compro = false;
Venta venta = {0, "", ""};
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


int vendo(int precio, char *vendedor, char *comprador) {
  pthread_mutex_lock(&m);

  //Alguien ya vende
  if(vendo) {
    if(venta.precio < precio) { //Precio actual de venta es menor
      //Liberar memoria y retornar falso
      pthread_mutex_unlock(&m);
      return false
    } else { //Precio actual de venta es mayor
      //Actualizar los datos de venta a los mios, despertar al vendedor dormido y liberar memoria
      venta.vendedor = *vendedor;
      venta.precio = precio;
      pthread_cond_broadcast(&cond);
    }
  }

  while(!compro) { //No hay nadie comprando aún
    venta.vendedor = *vendedor;
    venta.precio = precio;
    vendo = true;
    pthread_cond_wait(&cond, &m); //Esperar a un comprador o un precio de venta menor
  }
  //Despertado por un comprador

  //Despertado por un menor postor

  comprador = &venta.comprador;
  pthread_mutex_unlock(&m);
  return true;

}

int compro(char *comprador, char *vendedor) {
  pthread_mutex_lock(&m);
  if(vendo) { //Alguien quiere vender
    vendedor = &compra.vendedor;
    venta.comprador = *comprador;
    int precioVenta = venta.precio;
    vendo = false;
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&m);
    return precioVenta;
  } else { //Nadie vende aún
    compro = true;
    pthread_cond_wait(&cond, &m);
  }
}
