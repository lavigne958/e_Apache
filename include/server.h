#ifndef SERVER_H
#define SERVER_H

#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "vigilante.h"

typedef struct client{
  int socket;
  int log_file;
  struct sockaddr_in expediteur;
  sem_t *sem;
  vigilante vigil;
  
}client;

		       
#endif
