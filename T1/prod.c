#include <stdio.h>
#include <pthread.h>
#include "prod.h"

typedef struct {
  int *a;
  int i, j, p;
  BigNum *prod;
} Args;

void *parArrayProd_thread(void *ptr) {
  Args *args = (Args*) ptr;
  args -> prod = parArrayProd(args -> a, args -> i, args -> j, args -> p); 
  return NULL;
}

BigNum *parArrayProd(int a[], int i, int j, int p){
  if (i == j){ //Caso base i==j (trivial)
    return smallNum(a[i]); //Convierte int a[i] en BigNum
  } else{
    if (p == 1){ //Caso P=1
      BigNum *prod = seqArrayProd(a, i, j);
      return prod;
    } else { //Caso P>1
      //Un thread que se encargue de calcular el producto de i hasta h usando p/2 procesos
      pthread_t pid_1;
      int h = (i+j)/2;
      Args args_1 = {a, i, h, p/2, NULL};
      pthread_create(&pid_1, NULL, parArrayProd_thread, &args_1);
      //Thread original calcula recursivamente el producto de h+1 hasta j usando p-p/2 threads
      pthread_t pid_2;
      Args args_2 = {a, h+1, j, p-(p/2), NULL};
      pthread_create(&pid_2, NULL, parArrayProd_thread, &args_2);
      //Esperar
      pthread_join(pid_1, NULL);
      pthread_join(pid_2, NULL);
      //Calcular el producto
      BigNum *left = args_1.prod;
      BigNum *right = args_2.prod;
      BigNum *prod = bigMul(left, right);
      freeBigNum(right);
      freeBigNum(left);
      return prod;
    }
  }
}