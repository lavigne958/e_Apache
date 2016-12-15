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

#define SEM "/semaphore_e_apache_server"

int sock = -1;
int fd = -1;
sem_t *semaphore = NULL;

void close_sock(){
  
  printf("fermeture du server\n");
  if(sock != -1){
    printf("le socket n'est pas vide: %d\n", sock);
    close(sock);
  }
  if(fd != -1){
    close(fd);
  }

  if(semaphore){
    sem_close(semaphore);
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

  sem_unlink(SEM);
  perror(SEM);
  semaphore = sem_open(SEM, O_CREAT | O_RDWR | O_EXCL, 600, 1);

  if( semaphore == SEM_FAILED){
    perror("semaphore n'a pas pu etre créer");
    return errno;
  }

  if((fd = open("/tmp/http3200583.log", O_CREAT | O_TRUNC | O_RDWR | O_APPEND, 0600)) == -1){
      perror("[server]\topen log file");
      return EXIT_FAILURE;
  }
  
  while(1){
    sock_client = accept(sock, (struct sockaddr*)&exp, &explen);

    if(sock_client == -1){
      perror("accept new client");
      return errno;
    }
    printf("[server]\tnouvelle connexion d'un client\n");

    tmp = (client*) malloc(sizeof(client));

    tmp->socket = sock_client;
    tmp->expediteur = exp;
    tmp->log_file = fd;
    tmp->sem = semaphore;

    printf("[server]\tlancement du thread\n");
    if(pthread_create(&t_id, NULL, process_request, (void*)tmp) != 0){
      perror("pthread_create");
      shutdown(sock_client, SHUT_RDWR);
    }

    printf("[server]\tthread créer, attente d'une nouvelle connexion\n");
  }
  return EXIT_SUCCESS;
}
