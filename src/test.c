#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(){
  char message[256];
  char chemin[256];
  char pattern[] = "Content-Type: %s";
  char query[] = "GET /chemin HTTP/1.1";
  sscanf(query, "GET %s", chemin);
  sprintf(message, pattern, "text/html");
  printf("%s\n", message);
  printf("%s\n", chemin);
  return EXIT_SUCCESS;
}
