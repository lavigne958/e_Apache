#define _XOPEN_SOURCE 700
#define _REENTRANT 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "vigilante.h"

vigilante *create_vigilante_thread(){
  vigilante *v = (vigilante*)malloc(sizeof(vigilante));
  v->count = 0;
  pthread_mutex_init(&v->mutex, NULL);
  if(pthread_create(&v->tid, NULL, vigilante_thread, (void*)v) != 0){
    perror("create_vigilante_thread");
    return NULL;
  }
  return v;
}

void incremente_size(vigilante *v, int size){
  if(!v){
    fprintf(stderr, "incremente: vigilante NULL\n");
    return;
  }
  printf("[vigilante]\t on incremente la taille de : %d\n", size);
  pthread_mutex_lock(&v->mutex);
  v->count += size;
  v->flag |= DIRTY;
  pthread_mutex_unlock(&v->mutex);
}


void read_size(vigilante *v){
  if(!v){
    fprintf(stderr, "incremente: vigilante NULL\n");
    return;
  }
  pthread_mutex_lock(&v->mutex);
  if((v->flag & DIRTY) == 0){
    printf("rien a lire\n");
    pthread_mutex_unlock(&v->mutex);
    return;
  }
  printf("[vigilante]\tsize = %d\n", v->count);
  v->flag &= ~DIRTY;
  pthread_mutex_unlock(&v->mutex);
}


void* vigilante_thread(void *vigil){
  struct timespec start;
  struct timespec end;
  struct timespec echeance;
  struct timespec temps_restant;
  vigilante v = *(vigilante*)vigil;
  while(1){
    /* temps avant traitemen */
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
      perror("clock_gettime");
      exit(1);
    }

    /*    printf("il est: %d - %d\n", (int)start.tv_sec, (int)start.tv_nsec);*/

    

    /* ----------- TRAITEMENT -----------*/
    read_size(&v);

    /* temps après traitement */
    if(clock_gettime(CLOCK_REALTIME, &end) == -1){
      perror("clock_gettime");
      exit(1);
    }

    echeance.tv_sec = 0;
    echeance.tv_nsec = 1E9 - ((end.tv_sec * 1E9 + end.tv_nsec) - (start.tv_sec * 1E9 + start.tv_nsec));
    /*    printf("fin: %d - %d -- temps restant: %d \n", (int)end.tv_sec, (int)end.tv_nsec, (int)echeance.tv_nsec/1000000);*/
    if(echeance.tv_nsec > 0){
      clock_nanosleep(CLOCK_REALTIME, 0, &echeance, &temps_restant);
    }
  }
}

