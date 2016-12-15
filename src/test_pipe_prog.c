#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
  printf("HTTP/1.1 200 OK\n\n");
  printf("<html>\n\t<head><title>Test d'execution de CGI</title>\n</head>\n");
  printf("<body>\n\t<h1>Test de CGI, IT Works Mate !</h1>\n\t<p>this is so good buddy</p>\n</body>\n</html>\n");
  sleep(12);
  return EXIT_SUCCESS;
}
