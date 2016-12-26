#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void vigilanty(){
  struct timespec start;
  struct timespec end;
  struct timespec echeance;
  struct timespec temps_restant;
  while(1){
    /* temps avant traitemen */
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
      perror("clock_gettime");
      exit(1);
    }

    printf("il est: %d - %d\n", start.tv_sec, start.tv_nsec);

    

    /* ----------- TRAITEMENT -----------*/
    

    /* temps après traitement */
    if(clock_gettime(CLOCK_REALTIME, &end) == -1){
      perror("clock_gettime");
      exit(1);
    }

    echeance.tv_sec = 0;
    echeance.tv_nsec = 1E9 - ((end.tv_sec * 1E9 + end.tv_nsec) - (start.tv_sec * 1E9 + start.tv_nsec));
    printf("fin: %d - %d -- temps restant: %d \n", end.tv_sec, end.tv_nsec, echeance.tv_nsec/1000000);
    if(echeance.tv_nsec > 0){
      clock_nanosleep(CLOCK_REALTIME, 0, &echeance, &temps_restant);
    }
  }
}

int main(){

  vigilanty();
}
