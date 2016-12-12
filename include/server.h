#IFNDEF SERVER_H
#define SERVER_H

#include <semaphore.h>

typedef struct client{
  short status; // égal à -1 si case libre
  int socket;
  int indice;
  sockaddr_in expediteur;
  sem_t *sem;
}client;

typedef struct freelist{
  int position;
  struct freelist* next;
} freelist;

		       
#endif
