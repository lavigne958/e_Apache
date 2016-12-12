#define _XOPEN_SOURCE 700

#include "server.h"

int verif_file(

void *process_request(void *arg){
  char message[SIZE_REQUEST];
  char *chemin;
  int i=-1;
  client self = *(client*)arg;

  do{
    i++;
    read(self.socket, &message[i], 1);
  }while(message[i] != '\n' && i < SIZE_REQUEST);

  if(i == SIZE_REQUEST){
    fprintf(stderr, "Request is too long [4K max]\n");
    exit(EXIT_FAILURE);
  }
  
  chemin = (char*) malloc(sizeof(char) * ++i);
  strncpy(chemin, message, i);
  printf("req1: %s\n", chemin);

  i = -1
  do{
    i++;
    read(self.socket, &message[i], 1);
  }while(message[i] != '\n' && i < SIZE_REQUEST);

  if(i == SIZE_REQUEST){
    fprintf(stderr, "Request is too long [4K max]\n");
    exit(EXIT_FAILURE);
  }

  ++i;
  message[i] = '\0';

  printf("req2: %s\n", message);
  

  
  
  self.status = -1;
  pthread_exit((void*)EXIT_SUCCESS);
}
