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

#define SIZE_REQUEST 256


int sock = -1;
void close_sock(){
  if(sock != -1)
    close(sock);
}

/** Verifie que la requete se termine par deux "\n", 
    quelle soit bien de type "Get" et que host soit correct */
int check_request(const char *request, int size){
  return 1;
}

void *process_request(void *arg){
  int i, n, fd;
  char request[SIZE_REQUEST];
  char chemin[SIZE_REQUEST];
  int sock = (*(int*)arg);
  while(1){
    n = read(sock, request, SIZE_REQUEST);
    if(n == -1){
      perror("process_request: read");
      continue;
    }
    
    if(!check_request(request, n)){
      fprintf(stderr, "Error: bad formed request\n");
    }

    strcpy(chemin, "files/");
    sscanf(request, "GET %s", chemin + 7);

    if((fd = open(chemin, O_RDWR)) == -1){
      perror("process_request: open");
      continue;
    }

    while(1){
      n = read(fd, request, SIZE_REQUEST);
      if(n == -1){
	perror("process_request: read file");
	break;
      }
      if(n == 0){
	break;
      }
      write(sock, request, n);
    }
    
  }
  pthread_exit((void*)EXIT_SUCCESS);
}


int main(int argc, char *argv[]){
  int port, nb_clients, sock, client;
  int *temp;
  int option;
  pthread_t tid;
  
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
  
  while(1){
    client = accept(sock, (struct sockaddr*)&exp, &explen);
    temp = (int*)malloc(sizeof(int));
    *temp = client;
    if(pthread_create(&tid, NULL, process_request, (void*)temp) != 0){
      perror("pthread_create");
      return errno;
    }
  }
  return EXIT_SUCCESS;
}
