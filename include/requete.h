#ifndef REQUET_H
#define REQUET_H

#include "server.h"
#include "vigilante.h"

#define NOT_FOUND 404
#define FORBIDDEN 403
#define OK 200
#define INTERNAL_ERROR 500
#define SIZE_REQUEST 4096

typedef struct thread_fils{
  int *counter;
  int id;
  pthread_mutex_t *mutex;
  pthread_cond_t *cond;
  client *cli;
  char get[SIZE_REQUEST];
}thread_fils;

char* get_time(char* result);

void check_file(const char *pathname, int* code, char* string);

void *thread_server(void *arg);

void *process_request(void *arg);

int get_mime(char* extention, char* mime);

void write_log(client *c, char* str_get, int ret_code, int size);

int send_file(int socket, const char* pathname);

#endif
