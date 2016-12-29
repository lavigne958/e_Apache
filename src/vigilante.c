#define _XOPEN_SOURCE 700
#define _REENTRANT 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "vigilante.h"

vigilante *create_vigilante_thread(int threshold){
  vigilante *v = (vigilante*)malloc(sizeof(vigilante));
  
  pthread_mutex_init(&v->mutex, NULL);
  v->clients = NULL;
  v->threshold = threshold;
  
  if(pthread_create(&v->tid, NULL, vigilante_thread, (void*)v) != 0){
    perror("create_vigilante_thread");
    return NULL;
  }
  return v;
}

void add_client(vigilante *vigil, long ip){
  int i;
  client_data_count *current;
  client_data_count *new_client;

  pthread_mutex_lock(&vigil->mutex);
    
  current = vigil->clients;
  
  while(current != NULL && current->ip != ip)
    current = current->next;
  
  if(current == NULL){
    printf("[vigilante]\tle client: %ld n'existe pas dans la liste\n", (long) ip);
    new_client = (client_data_count*) malloc(sizeof(client_data_count));
    
    for(i = 0; i < 60; i++){
      new_client->track[i] = 0;
    }
    
    new_client->ip = ip;
    new_client->flag = 0;
    new_client->last = -1;
    new_client->timeleft = 0;
    current = vigil->clients;
    vigil->clients = new_client;
    new_client->next = current;
  }else{
    printf("[vigilante]\tle client: %ld existe déjà\n", (long) ip);
  }
  
  printf("[vigilante]\t ajout du client id: %ld\n", ip);
  pthread_mutex_unlock(&vigil->mutex);
}



int check_total(client_data_count *client){
  int i;
  int total = 0;

  for(i = 0; i < 60; i++) total+= client->track[i];

  return total;
}  

int incremente_size(vigilante *v, int size, long ip){
  client_data_count *self;
  struct tm *local_time;
  time_t current_time;
  int total;
  
  if(!v){
    fprintf(stderr, "incremente: vigilante NULL\n");
    return 0;
  }

  if(v->clients == NULL){
    fprintf(stderr, "incremente: vigilante->clients NULL\n");
    return 0;
  }

  current_time = time(NULL);
  local_time = localtime(&current_time);
  
  printf("[vigilante]\t on incremente la taille de : %d\n", size);
  pthread_mutex_lock(&v->mutex);

  self = v->clients;

  while(self != NULL && self->ip != ip) self = self->next;

  if(self == NULL){
    fprintf(stderr, "[vigilante]\tla liste ne contient pas l'id rechercher: %ld\n", ip);
    pthread_mutex_unlock(&v->mutex);
    return 0;
  }

  if(self->flag & BLOCKED){
    printf("[vigilante]\t %ld ne peut pas continuer de recevoir des données (max: %d)\n", (long)ip, v->threshold);
    self->timeleft = 10;
    pthread_mutex_unlock(&v->mutex);
    return 0;
  }

  total = check_total(self);
  printf("[vigilante]\t total accumulé: %d\n", total + size);

  if( (total + size) >= v->threshold){
    printf("[vigilante]\t %ld ne peut pas continuer de recevoir des données (max: %d)\n", (long)ip, v->threshold);
    self->flag |= BLOCKED;
    self->timeleft = 10;
    pthread_mutex_unlock(&v->mutex);
    return 0;
  }

  self->track[local_time->tm_sec] = size;
  self->last = local_time->tm_sec;
  
  pthread_mutex_unlock(&v->mutex);

  printf("[vigilante]\t %ld peut recevoir des données\n", (long)ip);
  return 1;
}


/* si aucune requete n'a été emise par un client durant la seconde 
   courante, cette méthode met la case du tableau correspondante
   à zero. Si l'on trouve un client bloqué on décrémente le champs timeleft
   représentant le temps restant avant son débloquage
*/
void check_clients(vigilante *v){
  client_data_count *current;
  struct tm *local_time;
  time_t current_time;
  int i;
  current_time = time(NULL);
  local_time = localtime(&current_time);

  printf("[vigilante]\t on efface la case: %d\n", local_time->tm_sec);

  pthread_mutex_lock(&v->mutex);

  current = v->clients;
  while(current != NULL){
    if(current->last != local_time->tm_sec){
      current->track[local_time->tm_sec] = 0;
      if(current->flag & BLOCKED){
	current->timeleft--;
	if(current->timeleft == 0){
	  current->flag &= ~BLOCKED;
	  for(i=0; i < 60; i++)
	    current->track[i] = 0;
	}
      }
    }
    current = current->next;
  }

  pthread_mutex_unlock(&v->mutex);
}


void* vigilante_thread(void *vigil){
  struct timespec start;
  struct timespec end;
  struct timespec echeance;
  struct timespec temps_restant;
  vigilante *v = (vigilante*)vigil;
  while(1){
    /* temps avant traitement */
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
      perror("clock_gettime");
      exit(1);
    }

    /* ----------- TRAITEMENT -----------*/
    check_clients(v);

    

    /* temps après traitement */
    if(clock_gettime(CLOCK_REALTIME, &end) == -1){
      perror("clock_gettime");
      exit(1);
    }

    echeance.tv_sec = 0;
    echeance.tv_nsec = 1E9 - ((end.tv_sec * 1E9 + end.tv_nsec) - (start.tv_sec * 1E9 + start.tv_nsec));
    if(echeance.tv_nsec > 0){
      clock_nanosleep(CLOCK_REALTIME, 0, &echeance, &temps_restant);
    }
  }
}

