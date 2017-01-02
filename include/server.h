#ifndef SERVER_H
#define SERVER_H

#include <semaphore.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "vigilante.h"

typedef struct client{
  int socket;                     /* Socket avec lequel on communique avec le client */
  int log_file;                   /* Descripteur du fichier de journalisation */
  struct sockaddr_in expediteur;  /* Informations sur le client */
  sem_t *sem;                     /* Semaphore pour l'accès concurrent au fichier de log */
  vigilante *vigil;               /* Adresse d'une structure permettant de communiquer avec le thread vigilante (voir vigilante.h) */  
}client;
		       
#endif
