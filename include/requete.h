#ifndef REQUET_H
#define REQUET_H

#include "server.h"

#define NOT_FOUND 404
#define FORBIDDEN 403
#define OK 200
#define INTERNAL_ERROR 500

char* get_time(char* result);

void check_file(const char *pathname, int* code, char* string);

void *process_request(void *arg);

int get_mime(char* extention, char* mime);

void write_log(client *c, char* str_get, int ret_code, int size);

int send_file(int socket, const char* pathname);

#endif
