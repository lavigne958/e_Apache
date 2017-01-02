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
#include <sys/time.h>

#include "requete.h"
#include "server.h"
#include "vigilante.h"

#define SEM "/semaphore_e_apache_server"
#define TIMEOUT 10

int sock = -1;
int fd = -1;
sem_t *semaphore = NULL;

/* methode appelee lors du traitement d'un signal SIGINT */
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
  int threshold;
  pthread_t t_id;

  struct sigaction action;
  struct sockaddr_in addr;
  struct sockaddr_in exp;
  struct timeval tv;
  client* tmp;
  vigilante *vigil;
  
  
  socklen_t explen = sizeof(exp);
  option = 1;

  if(argc != 4 ||
     ((port = atoi(argv[1])) == 0) ||
     ((nb_clients = atoi(argv[2])) == 0) ||
     ((threshold = atoi(argv[3])) == 0)){
    fprintf(stderr, "Usage: %s port clients threshold\n", argv[0]);
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

  /* set the time out for the read function to TIMEOUT value */
  tv.tv_sec = TIMEOUT;
  tv.tv_usec = 0;
  
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

  /* Creation du semaphore pour l'acces concurrent au fichier de log */
  sem_unlink(SEM);
  semaphore = sem_open(SEM, O_CREAT | O_RDWR | O_EXCL, 600, 1);

  if( semaphore == SEM_FAILED){
    perror("semaphore n'a pas pu etre créer");
    return errno;
  }

  /* Ouverture du fichier de log, ce descripteur sera transmis à tous les threads de
     gestion de connection.
  */
  if((fd = open("/tmp/http3200583.log", O_CREAT | O_TRUNC | O_RDWR | O_APPEND, 0600)) == -1){
      perror("[server]\topen log file");
      return EXIT_FAILURE;
  }

  /* On cree un thread vigilante qui surveillera que les donnees envoyees a chaque client
     n'excede pas threshold durant la derniere minute
  */
  vigil = create_vigilante_thread(threshold);
  while(1){
    printf("[server]\tthread créer, attente d'une nouvelle connexion\n");
    
    sock_client = accept(sock, (struct sockaddr*)&exp, &explen);

    if(sock_client == -1){
      perror("accept new client");
      return errno;
    }
    printf("[server]\tnouvelle connexion d'un client: %ld\n", (long)exp.sin_addr.s_addr);

    
    setsockopt(sock_client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval));

    tmp = (client*) malloc(sizeof(client));
    
    tmp->socket = sock_client;
    tmp->expediteur = exp;
    tmp->log_file = fd;
    tmp->sem = semaphore;
    tmp->vigil = vigil;
    
    if(pthread_create(&t_id, NULL, thread_server, (void*)tmp) != 0){
      perror("pthread_create");
      shutdown(sock_client, SHUT_RDWR);
    }
    
  }
  return EXIT_SUCCESS;
}
