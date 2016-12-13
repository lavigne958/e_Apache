#ifndef REQUET_H
#define REQUET_H

#define NOT_FOUND 404
#define FORBIDDEN 403
#define OK 200
#define INTERNAL_ERROR 500

void *process_request(void *arg);
int send_file(int socket, const char* pathname);

#endif
