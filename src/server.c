#define _XOPEN_SOURCE 700

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#include "requete.h"
#include "server.h"


int sock = -1;

void close_sock(){
  
  printf("fermeture du server\n");
  if(sock != -1){
    printf("le socket n'est pas vide: %d\n", sock);
    close(sock);
  }
  
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]){
  int port, sock, sock_client;
  int option;
  int nb_clients;
  pthread_t t_id;

  struct sigaction action;
  struct sockaddr_in addr;
  struct sockaddr_in exp;
  client* tmp;

  
  socklen_t explen = sizeof(exp);
  option = 1;

  if(argc != 4 || ((port = atoi(argv[1])) == 0) || ((nb_clients = atoi(argv[2])) == 0)){
    fprintf(stderr, "Usage: %s port clients unboundedvalue\n", argv[0]);
    return EXIT_FAILURE;
  }
   
  /** Redefinition du handler pour SIGINT */
 
  sigemptyset(&action.sa_mask);
  action.sa_flags = 0;
  action.sa_handler = close_sock;
  sigaction(SIGINT, &action, NULL);

  /** Creation de la socket  */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("creation socket");
    return errno;
  }
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

  memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  
  if(bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1){
    perror("bind socket");
    return errno;
  }

  listen(sock, nb_clients);
    
  while(1){
    sock_client = accept(sock, (struct sockaddr*)&exp, &explen);
    printf("[server]\tnouvelle connexion d'un client\n");

    tmp = (client*) malloc(sizeof(client));

    tmp->socket = sock_client;
    tmp->expediteur = exp;

    if(pthread_create(&t_id, NULL, process_request, (void*)tmp) != 0){
      perror("pthread_create");
      shutdown(sock_client, SHUT_RDWR);
    }

    printf("[server]\tthread créer, attente d'une nouvelle connexion\n");
  }
  return EXIT_SUCCESS;
}
