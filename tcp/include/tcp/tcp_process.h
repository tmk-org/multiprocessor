#ifndef _TCP_PROCESS_H_
#define _TCP_PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "misc/list.h"
#define MAX_BUFFER_SIZE 512

struct connection{
    list_t entry;
    //int broken;
    char request[MAX_BUFFER_SIZE];
    size_t request_used;
    char reply[MAX_BUFFER_SIZE];
    size_t reply_used;
    int fd;
};

//srv == NULL for client
struct connection *connection_add(void *ptr, int fd);
void connection_delete(void *ptr, struct connection *connection);

typedef char *(*reciever_func_t) (struct connection *connection, int *exit_flag);
typedef char *(*sender_func_t) (struct connection *connection, int *exit_flag);

//prepared request from client to server before write (usually this wait on input)
const char *default_client_sender(struct connection *connection, int *exit_flag);
//analized reply from server to client after read (usually this is analyzer for server replyes)
const char *default_client_reciever(struct connection *connection, int *exit_flag);
//prepared reply for write from server to client (usually this is callback for long actions ???)
const char *default_server_sender(struct connection *connection, int *exit_flag);
//analized request from client to server after read (usually this is command executor)
const char *default_server_reciever(struct connection *connection, int *exit_flag);

int tcp_server_process(char *port, char *(*server_reciever) (struct connection *, int *exit_flag), char *(*server_sender) (struct connection *, int *exit_flag));
int tcp_client_process(char* addr, char *port, char *(*client_sender) (struct connection *, int *exit_flag), char *(*client_reciever) (struct connection *, int *exit_flag));

#ifdef __cplusplus
}
#endif

#endif// _TCP_PROCESS_H_
