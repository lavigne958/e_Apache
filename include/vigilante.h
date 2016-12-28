#ifndef VIGILANTE_H
#define VIGILANTE_H

#include <pthread.h>
#define DIRTY 1

typedef struct vigilante{
  pthread_t tid;  /* id du thread vigilante lors d'un appel à create_vigilante_thread */
  int count;
  pthread_mutex_t mutex;
  int flag; /* indique si il y a eu changement */
}vigilante;

vigilante *create_vigilante_thread();

void incremente_size(vigilante *v, int size);

void read_size(vigilante *v);

void *vigilante_thread(void *vigil);

#endif
