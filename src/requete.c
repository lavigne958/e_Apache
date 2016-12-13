#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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

int get_mime(char* extention, char* mime){
  FILE* proc_file;
  char type[512];
  int proc;
  char s_command[] =
    "grep -w %s /etc/mime.types | awk '{if ($2 != \"\") if ($2 ~ /%s/) print $1}'";
  char command[100];

  sprintf(command, s_command, extention, extention);

  printf("[thread]\tcommand: %s\n", command);

  if( (proc_file = popen(command, "r")) != NULL){
    proc = fileno(proc_file);
    if( read(proc, type, 512) > 0){
      strcpy(mime, type);
      pclose(proc_file);
      return 1;
    }
  }
  pclose(proc_file);
  return 0;
}

void *process_request(void *arg){
  char message[SIZE_REQUEST];
  char pathname[SIZE_REQUEST];
  char *chemin;
  char *host;
  char *extension;
  char mime_type[50];
  int code;
  char str_code[10];
  client self = *(client*)arg;
  int i=-1;

  /* On lit la ligne du GET */
  do{
    i++;
    read(self.socket, &message[i], 1);
  }while(message[i] != '\n' && i < SIZE_REQUEST);

  if(i == SIZE_REQUEST){
    fprintf(stderr, "Request is too long [4K max]\n");
    shutdown(self.socket, SHUT_RDWR);
    pthread_exit((void*)EXIT_FAILURE);
  }

  /* On recupere le chemin du fichier demandé */
  chemin = (char*) malloc(sizeof(char) * ++i);
  sscanf(message, "GET %s HTTP/1.1\n", chemin);
  printf("[thread]\treq1: %s\n", chemin);

  /* On lit la ligne du host */
  i = -1;
  do{
    i++;
    read(self.socket, &message[i], 1);
  }while(message[i] != '\n' && i < SIZE_REQUEST);

  if(i == SIZE_REQUEST){
    fprintf(stderr, "Request is too long2 [4K max]\n");
    exit(EXIT_FAILURE);
  }

  /* On recupere le host */
  host = (char*)malloc(sizeof(char) * ++i);
  sscanf(message, "Host: %s\n", host);

  printf("[thread]\treq2: %s\n", host);

  if(chemin[0] == '/'){
    chemin = &chemin[1];
  }

  /* Si le chemin n'est composé que d'un / renvoyé le fichier index.html */
  if( strlen(chemin) == 0){
    strcpy(chemin, "index.html");
  }

  /* On recupere le repertoire courant est on le concatene 
     avec le chemin du fichier.
   */
  getcwd(pathname, SIZE_REQUEST);
  strcat(pathname, "/");
  strcat(pathname, chemin);

  /* Cette méthode renverra le code et le message
     de retour de notre server (200 "OK",...)
  */
  check_file(pathname, &code, str_code);
  sprintf(message, "HTTP/1.1 %d %s\n", code, str_code);
  write(self.socket, message, strlen(message));

  /* On recherche l'extension de notre fichier */
  extension = (char*) malloc(sizeof(char) * 5);
  extension = strstr(chemin, ".");
  extension++;

  /* On recupère le type du fichier */
  get_mime(extension, mime_type);
  sprintf(message, "Content-Type: %s\n\n", mime_type);
  write(self.socket, message, strlen(message));

  send_file(self.socket, pathname);

  shutdown(self.socket, SHUT_RDWR);
  pthread_exit((void*)EXIT_SUCCESS);
}


int send_file(int socket, const char* pathname){
  int fd;
  int n;
  char message[SIZE_REQUEST];
  if((fd = open(pathname, O_RDONLY)) == -1){
    perror("[thread] read_file: open");
    return EXIT_FAILURE;
  } 
  while(1){
    n = read(fd, message, SIZE_REQUEST);
    if(n == -1){
      perror("[thread] read_file: read");
      return errno;
    }
    if(n == 0){
      break;
    }
    write(socket, message, n);
  }
  return EXIT_SUCCESS;
}
