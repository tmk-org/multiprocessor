#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#include <pthread.h>

#define COMMAND_BUF_LEN 512

typedef struct command_t {
  char full_command[COMMAND_BUF_LEN];
  int client_sock;
} command_t;

static inline int command_server_init(){
    return 0;
}

static inline int add_command(command_t *command){
    return 0;
}

static inline int command_server_destroy() {
    return 0;
}

#endif /* _COMMAND_SERVER_H_ */
