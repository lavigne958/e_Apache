#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <wait.h>
 
#include "server.h"
#include "requete.h"

#define MS_TO_WAIT 10000
#define SIZE_MIME 50


char* get_time(char* result){
  struct tm *today;
  time_t current_time = time(NULL);
  today = localtime(&current_time);

  sprintf(result, "%d/%d/%d-%d:%d:%d ", today->tm_mday, today->tm_mon, today->tm_year, today->tm_hour, today->tm_min, today->tm_sec);

  return result;
}

void check_file(const char *pathname, int* code, char* string){
  struct stat buf;
  printf("[thread]\tpathname: %s\n", pathname);
  /* Si stat retourne ENOENT le fichier n'existe pas, 
     on renvoie 404 Not Found */
  if(stat(pathname, &buf) == -1){
    if(errno == ENOENT){
      *code = NOT_FOUND;
      strcpy(string, "Not Found");
      return;
    }
  }

  /* Si le serveur n'a pas les droits en lecture on
     renvoie 403 Forbidden
  */
  if((buf.st_mode & S_IRUSR) == 0){
    *code = FORBIDDEN;
    strcpy(string, "Forbidden");
    return;
  }

  /* Si le fichier est un executable */
  if((buf.st_mode & S_IXUSR)){
    *code = 0;
    return;
  }

  *code = OK;
  strcpy(string, "OK");
}

int get_mime(char* extention, char* mime){
  FILE* proc_file;
  char type[SIZE_MIME];
  int proc;
  char s_command[] =
    "grep -w %s /etc/mime.types | awk '{if ($2 != \"\") if ($2 ~ /%s/) print $1}'";
  char command[100];

  memset(type, '\0', SIZE_MIME);
  sprintf(command, s_command, extention, extention);

  if( (proc_file = popen(command, "r")) != NULL){
    proc = fileno(proc_file);
    if( read(proc, type, SIZE_MIME) > 0){
      strcpy(mime, type);
      pclose(proc_file);
      return 1;
    }
  }
  pclose(proc_file);
  return 0;
}

void write_log(client *c, char* str_get, int ret_code, int size)
{
  char temp[1024];
  char date[20];
  int fd = c->log_file;
  
  sprintf(temp, "%s - %s - %d - %ld - %s - %d - %d\n",
	  inet_ntoa(c->expediteur.sin_addr),
	  get_time(date),
	  getpid(),
	  (long)pthread_self(),
	  str_get,
	  ret_code,
	  size);	  

  sem_wait(c->sem);
  write(fd, temp, strlen(temp));
  write(fd, "\n", 1);
  sem_post(c->sem);
}

void execute(client *c, char *pathname, int *code, int *size){
  int pid;
  int status;
  int fd;
  int i;
  static int counter = 0;
  char tmp_file[50];

  sem_wait(c->sem);
  sprintf(tmp_file, "/tmp/exec_%d.tmp", ++counter);
  sem_post(c->sem);

  pid = fork();
  if(pid == -1){
    perror("Fork Error");
  }
  if(pid == 0){    
    if( (fd = open(tmp_file, O_CREAT | O_TRUNC | O_RDWR, 0600)) == -1){
      perror("impossible ouvir le fichier temporaire");
      return;
    }
    
    dup2(fd, STDOUT_FILENO);
    execl(pathname, pathname, NULL);
    perror("erreur execl");
    write(c->socket, "HTTP/1.1 500 Iternal Error\n\n", 28);

    exit(EXIT_FAILURE);
  }
  else{
    printf("[thread]\tprogramme: %s lancé attente de fin\n", pathname);
    i = 0;
    
    while(waitpid(pid, &status, WNOHANG) == 0 && (i++ < MS_TO_WAIT)){
      usleep(1000);
    }


    if(i >= MS_TO_WAIT || WEXITSTATUS(status) != 0){
      write(c->socket, "HTTP/1.1 500 Iternal Error\n\n", 28);
      *code = 500;
      *size = 0;
      kill(pid, SIGKILL);
    }else{
      send_file(c->socket, tmp_file);
    }
    unlink(tmp_file);
  }
}

void *process_request(void *arg){
  char message[SIZE_REQUEST];
  char pathname[SIZE_REQUEST];
  char *chemin;
  char *extension;
  char *str_get;
  char mime_type[SIZE_MIME];
  int code;
  int size;
  char str_code[10];
  struct stat stats;
  client self = *(client*)arg;
  int i=-1;

  memset(mime_type, '\0', SIZE_MIME);

  /* On lit la ligne du GET */
  do{
    i++;
    read(self.socket, &message[i], 1);
  }while(message[i] != '\n' && i < SIZE_REQUEST);

  if(i == SIZE_REQUEST){
    fprintf(stderr, "Request is too long [4K max]\n");
    fprintf(stderr, "received: %s\n\n", message);
    shutdown(self.socket, SHUT_RDWR);
    pthread_exit((void*)EXIT_FAILURE);
  }

  /* On recupere le chemin du fichier demandé */
  chemin = (char*) malloc(sizeof(char) * ++i);
  sscanf(message, "GET %s HTTP/1.1\r\n", chemin);

  str_get = (char*) malloc(sizeof(char) * i);
  strcpy(str_get, message);

  /* On lit la ligne du host */
  i = -1;
  do{
    i++;
    read(self.socket, &message[i], 1);
  }while(message[i] != '\n' && i < SIZE_REQUEST);

  if(i == SIZE_REQUEST){
    fprintf(stderr, "Request is too long2 [4K max]\n");
    fprintf(stderr, "received: %s\n\n", message);
    shutdown(self.socket, SHUT_RDWR);
    pthread_exit((void*)EXIT_FAILURE);
  } 

  if(chemin[0] == '/'){
    chemin = &chemin[1];
  }

  /* Si le chemin n'est composé que d'un / renvoyé le fichier index.html */
  if( strlen(chemin) == 0 ){
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
     ou 0 si le fichier est un executable.
  */
  check_file(pathname, &code, str_code);

  if(code == 0){
    execute(&self, pathname, &code, &size);
    write_log(&self, str_get, code, size);
  }
  else{
    sprintf(message, "HTTP/1.1 %d %s\r\n", code, str_code);
    write(self.socket, message, strlen(message));
    if(code != 200){
      printf("[thread]\tfile not found\n");
      write(self.socket, "\n", 1);
      write_log(&self, str_get, code, 0);
    }
    else{
      /* On recherche l'extension de notre fichier */
      extension = (char*) malloc(sizeof(char) * 5);
      extension = strstr(chemin, ".");
      extension++;

      memset(message, '\0', SIZE_REQUEST);
      
      /* On recupère le type du fichier */


      if( !get_mime(extension, mime_type) ){
	printf("[thread]\tmime failed\n");
	strcpy(mime_type, "text/plain");
      }
     
      /* pas besoin de \n la chaine mime_type l'a déjà à la fin */
      sprintf(message, "Content-Type: %s", mime_type);
      write(self.socket, message, strlen(message));
      stat(pathname, &stats);
      memset(message, '\0', SIZE_REQUEST);    
      sprintf(message, "Content-Length: %d\r\n\r\n", (int)stats.st_size);
      write(self.socket, message, strlen(message));
      
      send_file(self.socket, pathname);
      write_log(&self, str_get, code, stats.st_size);
    }
  }

  i = 0;

  printf("[thread]\t");
  while( (read(self.socket, message, 1) > 0) && i != 2){
    if(message[0] == '\n'){
      i++;
    }else{
      printf("%c", message[0]);
      i = 0;
    }
  }

  printf("\n");
  printf("[thread]\tfin de lecture des header non traité\n");

  shutdown(self.socket, SHUT_RDWR);

  /* if code == 0 (execution of a programm) then no need to read,
     the socket is already closed, for some reasons, I dunno,
     just ask Mr Vallade, president and chaiman of the project,
     please just don't put that on me... no my fault.... my bad*/
  read(self.socket, message, SIZE_REQUEST);
  close(self.socket);

  printf("[thread]\tlibération de la mémoire\n");
  if(arg) free(arg);  
  pthread_exit((void*)EXIT_SUCCESS);
}


int send_file(int socket, const char* pathname){
  int fd;
  int n;
  int nn;
  char message[SIZE_REQUEST];
  int send_taille = 0;
  if((fd = open(pathname, O_RDONLY)) == -1){
    perror("[thread] read_file: open");
    return EXIT_FAILURE;
  } 
  while(1){
    n = read(fd, message, SIZE_REQUEST);
    if(n == -1){
      perror("[thread]\tread_file: read");
      return errno;
    }

    nn = send(socket, message, n, 0);
    send_taille += nn;
    if(n == 0){
      break;
    }
  }
  close(fd);
  return EXIT_SUCCESS;
}
