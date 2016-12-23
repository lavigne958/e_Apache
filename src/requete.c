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

void *thread_server(void *arg){
  char message[SIZE_REQUEST];
  char *get;
  char host[SIZE_REQUEST];
  pthread_t t_id;
  pthread_cond_t *cond;
  int id;
  int i;
  int size;
  int *counter;
  thread_fils *fils;
  client *self;
  
  pthread_mutex_t *mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(mutex, NULL);

  cond = (pthread_cond_t*) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(cond, NULL);
  
  counter = (int*) malloc(sizeof(int));
  *counter = 0;
  id = 0;
  
  self = (client*) arg;
  get = (char*)malloc(sizeof(char) * SIZE_REQUEST);
  while(1){
    memset(get, '\0', SIZE_REQUEST);
    i = -1;
    
    /* On lit la ligne du GET */
    do{
      i++;
      size = read(self->socket, &get[i], 1);
    }while( get[i] != '\n' && i < SIZE_REQUEST && size > 0);
    
    if(i == SIZE_REQUEST){
      fprintf(stderr, "Request is too long [4K max]\n");
      fprintf(stderr, "received: %s\n\n", message);
      shutdown(self->socket, SHUT_RDWR);
      pthread_exit((void*)EXIT_FAILURE);
    }

    printf("[thread]\ton à lu: %c - %d\n", get[i], size);

    if(size == 0){
      printf("[thread]\trien à lire, on quitte\n");
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }else if(size == -1){
      perror("lecture du GET");
      printf("[thread]\terreur lors de la lecture du socket\n");
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }
      
    printf("[thread]\tligne get: %d\n", i);
    get[i] = '\0';
    
    /* On lit la ligne du host */
    i = -1;
    do{
      i++;
      size = read(self->socket, &host[i], 1);
    }while(host[i] != '\n' && i < SIZE_REQUEST && size > 0);
    
    if(i == SIZE_REQUEST){
      fprintf(stderr, "Request is too long2 [4K max]\n");
      fprintf(stderr, "received: %s\n\n", message);
      shutdown(self->socket, SHUT_RDWR);
      pthread_exit((void*)EXIT_FAILURE);
    }
    
    host[i] = '\0';
    
    /* this section reads up the buffer
       up to the point it has twice '\n' red
       so we know that this is the end of the header
    */
    i = 1;
    
    do{
      size = read(self->socket, message, 1);
      printf("%c", message[0]);
      if(message[0] == '\n'){
	i++;
      }else if(message[0] != '\r'){
	i = 0;
      }
    }while(i < 2 && size > 0);
    
    printf("fin lecture headers\n");
    
    fils = (thread_fils*) malloc(sizeof(thread_fils));
    
    fils->counter = counter;
    fils->id = id++;
    fils->mutex = mutex;
    fils->cond = cond;
    fils->cli = self;
    strncpy(fils->get, get, strlen(get));
    
    printf("[server]\tlancement du sous-thread\n");
    if(pthread_create(&t_id, NULL, process_request, (void*)fils) != 0){
      perror("pthread_create");
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }
  }
}

void *process_request(void *arg){
  
  char pathname[SIZE_REQUEST];
  char *chemin;
  char *extension;
  char buffer[SIZE_REQUEST];
  char mime_type[SIZE_MIME];
  int code;
  int size;
  char str_code[10];
  struct stat stats;
  thread_fils self = *(thread_fils*)arg;

  memset(buffer, '\0', SIZE_REQUEST);

  /* On recupere le chemin du fichier demandé */
  chemin = (char*) malloc(sizeof(char) * strlen(self.get));
  size = sscanf(self.get, "%s %s HTTP/1.1\r\n",buffer, chemin);
  printf("[thread]\t\tmatches: %d\n", size);

  if(strcmp("GET", buffer) != 0){
    printf("[thread]\t\tCommande inconnue: %s - |commande|: %d\n", buffer, (int)strlen(buffer));
    shutdown(self.cli->socket, SHUT_RDWR);
    close(self.cli->socket);
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

  pthread_mutex_lock(self.mutex);

  while( *self.counter != self.id){
    pthread_cond_wait(self.cond, self.mutex);
  }

  if(code == 0){
    execute(self.cli, pathname, &code, &size);
    write_log(self.cli, self.get, code, size);
  }
  else{
    sprintf(buffer, "HTTP/1.1 %d %s\r\n", code, str_code);
    write(self.cli->socket, buffer, strlen(buffer));
    if(code != 200){
      printf("[thread]\t\t%d: %s\n", code, str_code);
      write(self.cli->socket, "\n", 1);
      write_log(self.cli, self.get, code, 0);
    }
    else{
      /* On recherche l'extension de notre fichier */
      extension = (char*) malloc(sizeof(char) * 5);
      extension = strstr(chemin, ".");
      extension++;

      /* On recupère le type du fichier */

      
      if( !get_mime(extension, mime_type) ){
	printf("[thread]\t\tmime failed\n");
	strcpy(mime_type, "text/plain");
      }
     
      /* pas besoin de \n la chaine mime_type l'a déjà à la fin */
      sprintf(buffer, "Content-Type: %s", mime_type);
      write(self.cli->socket, buffer, strlen(buffer));
      stat(pathname, &stats);
      sprintf(buffer, "Content-Length: %d\r\n\r\n", (int)stats.st_size);
      write(self.cli->socket, buffer, strlen(buffer));
      
      send_file(self.cli->socket, pathname);
      write_log(self.cli, self.get, code, stats.st_size);
    }
  }

  pthread_cond_broadcast(self.cond);

  pthread_mutex_unlock(self.mutex);

  printf("[thread]\t\tlibération de la mémoire\n");
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
