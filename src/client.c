define _XOPEN_SOURCE 700

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

int main(int argc, char *argv[]){
  int port, sock;
  
  struct sigaction action;
  struct sockaddr_in dest;
  char query[] = "GET files/Makefile HTTP/1.1\nHost: 127.0.0.1\n\n";
  
  if(argc != 2 || ((port = atoi(argv[1])) == 0)){
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

  memset(&dest, '\0', sizeof(dest));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  
  if(connect(sock, (struct sockaddr *) &dest, sizeof(dest)) == -1){
    perror("connect");
    return errno;
  }

  while(1){
    
  }
  return EXIT_SUCCESS;
}

