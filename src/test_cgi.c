#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(){
  printf("HTTP/1.1 200 OK\n");
  printf("Content-Length: %d\n\n",
    (int)strlen("<html>\n\t<head><title>Test d'execution de CGI</title>\n</head>\n<body>\n\t<h1>Test CGI, Cette page a été générée par un script C</h1>\n\t\n</body>\n</html>\n")); 
  printf("<html>\n\t<head><title>Test d'execution de CGI</title>\n</head>\n");
  printf("<body>\n\t<h1>Test CGI, Cette page a été générée par un script C</h1>\n\t\n</body>\n</html>\n");
  return EXIT_SUCCESS;
}
