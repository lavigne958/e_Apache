#ifndef SERVER_H
#define SERVER_H

#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SIZE_REQUEST 4096

typedef struct client{
  int socket;
  struct sockaddr_in expediteur;
  sem_t *sem;
}client;

		       
#endif
