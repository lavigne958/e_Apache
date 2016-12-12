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

#define SIZE_REQUEST 4096

int sock = -1;
client *tab_clients == NULL;
pthread_t *tab_tid == NULL;
int nb_clients;

void close_sock(){
  int i;
  if(sock != -1)
    close(sock);
  if(tab_tid){
    free(tab_tid);
  }
  if(tab_clients){
    free(tab_clients);
  }
}

freelist *push(int pos, freelist *liste){
  freelist *tmp = (freelist*)malloc(sizeof(freelist));
  tmp->position = pos;
  tmp->next = liste;
  return tmp;
}

freelist *pull(int *pos, freelist *liste){
  freelist *tmp = liste->next;
  *pos = liste->position;
  free(liste);
  return tmp;
}




int main(int argc, char *argv[]){
  int port, sock, client;
  int *temp, i;
  int option;
  
  int pos;

  struct sigaction action;
  struct sockaddr_in addr;
  struct sockaddr_in exp;
  socklen_t explen = sizeof(exp);
  option = 1;

  if(argc != 4 || ((port = atoi(argv[1])) == 0) || ((nb_clients = atoi(argv[2])) == 0)){
    fprintf(stderr, "Error: wrong arguments\n");
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

  tab_tid = (pthread_t*)malloc(sizeof(pthread_t) * nb_clients);
  tab_clients = (client*)malloc(sizeof(client) * nb_clients);
  if(!tab_clients){
    fprintf(stderr, "Error malloc\n");
    return EXIT_FAILURE;
  }
  
  for(i=0; i < nb_clients; i++){
    tab_clients[i].status = -1;
  }
  
  while(1){
    client = accept(sock, (struct sockaddr*)&exp, &explen);

    pos = 0;
    while(tab_client[pos].status != -1 && pos < nb_clients){
      i++;
    }
    client[pos].status = 1;
    client[pos].socket = client;
    client[pos].indice = pos;
    client[pos].expediteur = exp;

    if(pthread_create(&tab_tid[pos], NULL, process_request, (void*)&client[pos]) != 0){
      perror("pthread_create");
      return errno;
    }
  }
  return EXIT_SUCCESS;
}
