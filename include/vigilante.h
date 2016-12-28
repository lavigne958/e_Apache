#ifndef VIGILANTE_H
#define VIGILANTE_H

#include <pthread.h>
#define DIRTY 1

typedef struct client_data_count{
  int track[60];
  long ip;
  struct client_data_count *next;
  int flag;
  int last;
}client_data_count;

typedef struct vigilante{
  pthread_t tid;  /* id du thread vigilante lors d'un appel à create_vigilante_thread */
  int threshold;
  pthread_mutex_t mutex;
  client_data_count *clients;
}vigilante;

vigilante *create_vigilante_thread();

void add_client(vigilante* vigil, long ip);

int incremente_size(vigilante *v, int size, long ip);

void *vigilante_thread(void *vigil);

void check_clients(vigilante *v);

#endif
