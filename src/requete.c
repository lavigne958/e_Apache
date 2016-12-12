#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "server.h"
#include "requete.h"


void check_file(const char *pathname, int* code, char* string){
  struct stat buf;
  printf("[thread]\tpathname: %s\n", pathname);
  if(stat(pathname, &buf) == -1){
    if(errno == ENOENT){
      *code = NOT_FOUND;
      strcpy(string, "Not Found");
      return;
    }
  }

  if((buf.st_mode & S_IRUSR) == 0){
    *code = FORBIDDEN;
    strcpy(string, "Forbidden");
    return;
  }

  *code = OK;
  strcpy(string, "OK");
}

void get_mime(char* extention, char* mime){
  int proc;
  char type[512];
  char command =
    "grep -w %s /etc/mime.types | awk '{if ($2 != "") if ($2 ~ /%s/) print $1}'";

  sprintf(command, command, extention, extention);

  printf("[thread]\tcommand: %s\n", command);

  if( (proc = popen("grep -w html /etc/mime.types | awk '{if ($2 != "") print $1}'", "r");) != NULL){
    if( read(proc, type, 512) > 0){
      strcpy(mime, type);
      pclose(proc);
      return 1;
    }else{
      pclose(proc);
      return 0;
    }
  }
}

void *process_request(void *arg){
  char message[SIZE_REQUEST];
  char *chemin;
  char *host;
  int code;
  char str_code[10];
  char reponse[] = "HTTP/1.1 %d %s\n";
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
  sscanf(message, "GET %s HTTP/1.1\n", chemin);
  printf("[thread]\treq1: %s\n", chemin);

  i = -1;
  do{
    i++;
    read(self.socket, &message[i], 1);
  }while(message[i] != '\n' && i < SIZE_REQUEST);

  if(i == SIZE_REQUEST){
    fprintf(stderr, "Request is too long2 [4K max]\n");
    exit(EXIT_FAILURE);
  }

  host = (char*)malloc(sizeof(char) * ++i);
  sscanf(message, "Host: %s\n", host);

  printf("[thread]\treq2: %s\n", host);

  if(chemin[0] == '/'){
    chemin = &chemin[1];
  }

  if( strlen(chemin) == 0){
    strcpy(chemin, "index.html");
  }

  getcwd(message, SIZE_REQUEST);
  strcat(message, "/");
  strcat(message, chemin);

  check_file(message, &code, str_code);
  
  sprintf(message, reponse, code, str_code);

  

  write(self.socket, message, strlen(message));

  

  shutdown(self.socket, SHUT_RDWR);
  
  self.status = -1;
  pthread_exit((void*)EXIT_SUCCESS);
}
