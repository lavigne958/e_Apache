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
#include <regex.h>
 
#include "server.h"
#include "requete.h"
#include "vigilante.h"

#define MS_TO_WAIT 10000
#define SIZE_MIME 50


/* Cette méthode renvoie dans la chaine passée en paramètre la date
   courante sous le format day/month/year-hour:min:sec
*/
char* get_time(char* result){
  struct tm *today;
  time_t current_time = time(NULL);
  today = localtime(&current_time);

  sprintf(result, "%d/%d/%d-%d:%d:%d ", today->tm_mday, today->tm_mon, (today->tm_year + 1900), today->tm_hour, today->tm_min, today->tm_sec);

  return result;
}


/* Cette méthode vérifie que le fichier existe et qu'il est accessible par le serveur en
   lecture. Il renvoie dans les paramètres code et string, le code et le message
   de retour HTTP correspondant. Si le fichier est exécutable le code est égal à 0.
*/
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

/* Cette méthode recherche l'extension de la chaine "chemin"
   et la renvoie dans le paramètre extension
*/
char *get_extension(char *chemin, char *extension){
  int i, j;
  char *temp = (char*)malloc(sizeof(char) * strlen(chemin));
  j = 0;
  temp[0] = '\0';
  
  if(chemin == NULL || extension == NULL){
    fprintf(stderr, "Error get_extension: null pointer\n");
    return NULL;
  }

  /* On parcourt la chaine chemin depuis la fin jusqu'à trouver 
     un point
  */
  for(i=strlen(chemin)-1; i >= 0 && chemin[i] != '.'; i--){
    temp[j] = chemin[i];
    j++;
  }
  
  /* On a trouvé aucune extension */
  if(i < 0){
    extension[0] = '\0';
    free(temp);
    return NULL;
  }

  /* On remplit la chaine extension avec le contenu de temp */
  for(i=0, j--; j >= 0; i++, j--){
    extension[i] = temp[j];
  }
  extension[i] = '\0';
  free(temp);
  return extension;
}



/* Cette méthode parse le fichier /etc/mime.types pour chercher le mime type 
   correspondant à l'extension et renvoie le resultat dans la chaine 
   mime passée en paramètres.
   Si la fonction n'a rien trouvée elle renvoie 0.
*/
int get_mime(const char *extension, char *mime){
  regex_t preg;
  char line[512];
  FILE *f;
  char *str_regex;

  int match;
  size_t nmatch;
  regmatch_t *pmatch;
  int start;
  int end;
  size_t size;
  
  const char template[] = "([[:alnum:]]+/[[:alnum:]+-.]+).*[[:space:]]+%s[[:space:]]";

  if(extension == NULL || extension[0] == '\0'){
    return 0;
  }
  
  str_regex = (char*)malloc(sizeof(char) * (strlen(template) + strlen(extension)));
  sprintf(str_regex, template, extension);
  
  f = fopen("/etc/mime.types", "r");
  if(!f){
    printf("Erreur: ouverture fichier mime.types\n");
    fclose(f);
    return 0;
  }

  /* Compilation de la regex */
  if(regcomp (&preg, str_regex, REG_EXTENDED)){
    fclose(f);
    return 0;
  }
  
  /* On alloue un tableau qui contiendra les informations
     sur les correspondances entre la chaine et l'expression reguliere */
  pmatch = NULL;
  nmatch = preg.re_nsub + 1;
  pmatch = malloc (sizeof (*pmatch) * nmatch);
  if (!pmatch){
    fprintf(stderr, "Erreur malloc\n");
    return 0;
  }

  /* On parcourt chaque ligne du fichier pour trouver le mime type */
  while(fgets(line, 512, f) != NULL){
    match = regexec (&preg, line, nmatch, pmatch, 0);
    if (match == 0){
      start = pmatch[1].rm_so;
      end = pmatch[1].rm_eo;
      size = end - start;
      if (mime){
	strncpy (mime, &line[start], size);
	mime[size] = '\0';
      }
      printf("TYPE: %s\n", mime);
      free(pmatch);
      fclose(f);
      return 1;
    }
  } 
  free(pmatch);
  fclose(f);
  return 0;
}


/* Cette methode va chercher dans un fichier contenant un message de reponse
   http, le code de retour et la taille des donnees envoyees
*/
int get_code_and_size(const char *filename, int *code, int *length){
  regex_t reg_http;
  regex_t reg_length;
  char line[256];
  char result[256];
  FILE *f;

  int match;
  int nbmatch;
  regmatch_t *tab_match;
  int start;
  int end;
  size_t size;

  char* regex_length;
  char* regex_http;

  regex_http = "HTTP/1.1 ([[:digit:]]{3}) ";
  regex_length = "Content-Length: ([[:digit:]]+)";

  f = fopen(filename, "r");
  if(!f){
    printf("Erreur: ouverture fichier %s\n", filename);
    fclose(f);
    return 0;
  }

  /* Compilation des regex */
  if(regcomp (&reg_http, regex_http, REG_EXTENDED) != 0){
    fclose(f);
    return 0;
  }

  if(regcomp (&reg_length, regex_length, REG_EXTENDED) != 0){
    fclose(f);
    return 0;
  }

  /* On alloue un tableau qui contiendra les informations
     sur les correspondances entre la chaine et l'expression reguliere */
  tab_match = NULL;
  nbmatch = 2;
  tab_match = malloc (sizeof (*tab_match) * nbmatch);
  
  if (!tab_match){
    fprintf(stderr, "Erreur malloc\n");
    return 0;
  }

  /* Si le fichier est vide on retourne 0 */
  if(fgets(line, 256, f) == NULL){
    fclose(f);
    free(tab_match);
    return 0;
  }
  
  /* On recherche le code de retour de la reponse. */
  match = regexec(&reg_http, line, nbmatch, tab_match, 0);

  /* Mauvaise reponse http si on ne trouve aucune correspondance */
  if(match != 0){
    free(tab_match);
    fclose(f);
    return 0;
  }

  /* On retourne le code http dans l'argument code */
  start = tab_match[1].rm_so;
  end = tab_match[1].rm_eo;
  size = end - start;
  strncpy(result, &line[start], size);
  result[size] = '\0';
  *code = atoi(result);
  
  /* Tant qu'il y a des headers a lire on recherche
     le header Content-Length
  */
  while(fgets(line, 256, f) != NULL && strlen(line) != 1){

    match = regexec (&reg_length, line, nbmatch, tab_match, 0);
    if (match != 0){
      continue;
    }
    start = tab_match[1].rm_so;
    end = tab_match[1].rm_eo;
    size = end - start;
    strncpy (result, &line[start], size);
    result[size] = '\0';
    *length = atoi(result);
    printf("function size: %d\n", *length);
    free(tab_match);
    fclose(f);
    return 1;
  }

  free(tab_match);

  size = 0;
  /* Si ce header n'est pas present, il faut compter 
     la taille des donnees envoyees */
  while(fgets(line, 256, f) != NULL){
    size += strlen(line);
  }
  *length = size;
  
  fclose(f);
  return 1;
}

/*
  Cette méthode va ajouter une ligne dans le fichier de journalisation 
  avec les arguments passées en paramètres. 
*/
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

/* Cette méthode est celle exécutée par les threads créées par le serveur
   pour gérer une connection. Pour chaque requêtes, elle lira tous les headers
   et créera une thread qui s'occupera de traiter la requête.
*/
void *thread_server(void *arg){
  char message[SIZE_REQUEST];
  char *get;
  char host[SIZE_REQUEST];
  pthread_t t_id;
  pthread_cond_t *cond;
  int id;
  int i;
  int size;
  int code;
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
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }

    if(size == 0){
      printf("[thread]\trien à lire, on quitte\n");
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }else if(size == -1){
      perror("lecture du GET");
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }
    
    get[i] = '\0';
    
    /* On lit la ligne du host */
    i = -1;
    do{
      i++;
      size = read(self->socket, &host[i], 1);
    }while(host[i] != '\n' && i < SIZE_REQUEST && size > 0);
    
    if(i == SIZE_REQUEST){
      fprintf(stderr, "Request is too long [4K max]\n");
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }
    
    host[i] = '\0';
    
    /* On lit tous les headers, on arrête donc la boucle
       à la réception de deux "\n" ou de deux "\r\n"
    */
    i = 1;
    do{
      size = read(self->socket, message, 1);
      if(message[0] == '\n'){
	i++;
      }else if(message[0] != '\r'){
	i = 0;
      }
    }while(i < 2 && size > 0);
    
    add_client(self->vigil, self->expediteur.sin_addr.s_addr);
    
    if( is_blocked(self->vigil, self->expediteur.sin_addr.s_addr)){
      printf("[thread]\tle client: %d est déjà banis\n", self->expediteur.sin_addr.s_addr);
      code = 403;
      size = 0;
      sprintf(message, "HTTP/1.1 403 Forbidden\r\n\r\n");
      write(self->socket, message, strlen(message));
      write_log(self, get, code,  size);
      shutdown(self->socket, SHUT_RDWR);
      close(self->socket);
      pthread_exit((void*)EXIT_FAILURE);
    }
    
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


/* Cette méthode est celle exécutée par les sous-threads pour traiter 
   une requête.
*/
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
  
  int pid;
  int status;
  int fd;
  int i;
  char tmp_file[50];
  static unsigned long int counter = 0;
  
  thread_fils self = *(thread_fils*)arg;
  memset(buffer, '\0', SIZE_REQUEST);

  /* On recupere le chemin du fichier demandé */
  chemin = (char*) malloc(sizeof(char) * strlen(self.get));
  size = sscanf(self.get, "%s %s HTTP/1.1\n",buffer, chemin);

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

  /*
    Si le code est égal à 0 la requête concerne un fichier exécutable. 
    Le processus fils qui exécutera le code ecrira le resultat de l'exécution dans 
    un fichier temporaire (code de retour HTTP + données).
    Si l'exécution se déroule en moins de MS_TO_WAIT ms, le processus père
    enverra tous le contenu du fichier au socket client, sinon il envoie 
    "HTTP/1.1 500 Iternal Error\n\n".
  */
  if(code == 0){
    /* On crée le nom du fichier temporaire */
    sem_wait(self.cli->sem);
    sprintf(tmp_file, "/tmp/exec_%ld.tmp", ++counter);
    sem_post(self.cli->sem);

    pid = fork();
    if(pid == -1){
      perror("Fork Error");
      pthread_exit(&errno);
    }
    if(pid == 0){    
      if( (fd = open(tmp_file, O_CREAT | O_TRUNC | O_RDWR, 0600)) == -1){
	perror("impossible ouvir le fichier temporaire");
	exit(EXIT_FAILURE);
      }
      /* On redirige la sortie standard du processus vers le fichier 
	 avant l'exécution.
      */
      dup2(fd, STDOUT_FILENO);
      execl(pathname, pathname, NULL);
      perror("erreur execl");

      exit(EXIT_FAILURE);
    }
    else{
      i = 0;
      /* Toutes les microsecondes le processus père vérifie que le processus fils
	 s'est terminé ou si MS_TO_WAIT ms ce sont déroulées
      */
      while(waitpid(pid, &status, WNOHANG) == 0 && (i++ < MS_TO_WAIT)){
	usleep(1000);
      }
      /* Si le fils s'est mal terminé ou pas dans les temps */
      if(i >= MS_TO_WAIT || WEXITSTATUS(status) != 0){
	while( *self.counter != self.id){
	  pthread_cond_wait(self.cond, self.mutex);
	}
	write(self.cli->socket, "HTTP/1.1 500 Iternal Error\n\n", 28);
	code = 500;
	size = 0;
	kill(pid, SIGKILL);
      }
      /* Si le fils s'est correctement termine */
      else{
	while(*self.counter != self.id){
	  pthread_cond_wait(self.cond, self.mutex);
	}
	/* On va recuperer dans le fichier le code de retour et la taille de
	   la reponse du fils, si la reponse est correcte et que le client a le droit 
	   de recevoir des donnees, on lui envoie le contenu du fichier
	*/
	if(get_code_and_size(tmp_file, &code, &size) != 0){
	  /* On verifie que la taille des données téléchargées dans la dernière minute
	     + la taille de la reponse à la requête courante n'excède pas le seuil. */
	  if(incremente_size(self.cli->vigil, size, self.cli->expediteur.sin_addr.s_addr)){
	    send_file(self.cli->socket, tmp_file);
	  }
	  else{
	    code = 403;
	    size = 0;
	    sprintf(buffer, "HTTP/1.1 403 Forbidden\r\n\r\n");
	    write(self.cli->socket, buffer, strlen(buffer));
	  }
	}
	else{
	  write(self.cli->socket, "HTTP/1.1 500 Iternal Error\n\n", 28);
	  code = 500;
	  size = 0;
	}
	unlink(tmp_file);
      }
      printf("[thread_fils]\tsize: %d\n", size);
      write_log(self.cli, self.get, code, size);
    }
  }
  else{
    if(code != 200){
      sprintf(buffer, "HTTP/1.1 %d %s\n\n", code, str_code);
      printf("thread erreur: %s\n", buffer);
      while( *self.counter != self.id){
	pthread_cond_wait(self.cond, self.mutex);
      }
      write(self.cli->socket, buffer, strlen(buffer));
      write(self.cli->socket, "\n", 1);
      write_log(self.cli, self.get, code, 0);
    }
    else{
      
      /* On recherche l'extension de notre fichier */
      extension = (char*) malloc(sizeof(char) * 5);
      get_extension(chemin, extension);

      /* On recupère le type du fichier */

      if( !get_mime(extension, mime_type) ){
	printf("[thread]\t\tmime failed\n");
	strcpy(mime_type, "text/plain");
      }

      stat(pathname, &stats);
	
      /* On verifie que la taille des données téléchargées dans la dernière minute
	 + la taille de la reponse à la requête courante n'excède pas le seuil. */
      if(incremente_size(self.cli->vigil, stats.st_size, self.cli->expediteur.sin_addr.s_addr)){
	sprintf(buffer, "HTTP/1.1 %d %s\r\n", code, str_code);

	while(*self.counter != self.id){
	  pthread_cond_wait(self.cond, self.mutex);
	}
	write(self.cli->socket, buffer, strlen(buffer));
	
	sprintf(buffer, "Content-Type: %s\n", mime_type);
	write(self.cli->socket, buffer, strlen(buffer));
	
	sprintf(buffer, "Content-Length: %d\r\n\r\n", (int)stats.st_size);
	write(self.cli->socket, buffer, strlen(buffer));
	
	send_file(self.cli->socket, pathname);
      }
      else{
	sprintf(buffer, "HTTP/1.1 403 Forbidden\r\n\r\n");
	write(self.cli->socket, buffer, strlen(buffer));
      }

      write_log(self.cli, self.get, code, stats.st_size);
    }
  }
  
  *(self.counter) = *(self.counter) + 1;
  pthread_cond_broadcast(self.cond);

  pthread_mutex_unlock(self.mutex);
  
  pthread_exit((void*)EXIT_SUCCESS);
}

/* Cette méthode envoie le contenu d'un fichier au socket 
   passé en argument
*/
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
