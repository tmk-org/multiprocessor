#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#define COMMAND_QUEUE_SIZE 20
#define COMMAND_BUF_LEN 512

typedef struct command {
  char full_command[COMMAND_BUF_LEN];
  int fd; //client socket
} command_t;

int command_server_init(int *exit_flag);
int add_command(command_t *command);
int command_server_destroy();

#endif // _COMMAND_SERVER_H_
