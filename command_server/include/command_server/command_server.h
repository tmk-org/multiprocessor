#ifndef _COMMAND_SERVER_H_
#define _COMMAND_SERVER_H_

#define COMMAND_QUEUE_SIZE 20
#define COMMAND_BUF_LEN 512

typedef struct command {
    int fd;  //client socket
    char full_command[COMMAND_BUF_LEN];
    int with_reply;
    char reply[COMMAND_BUF_LEN];
} command_t;

struct command_server {
   struct ring_buffer *cmd_ring;
   pthread_t thread_id;
   //void (*executor) (command_t *cmd, int *exit_flag);
   int *exit_flag;
};

struct command_server *command_server_init(int *exit_flag);
int command_server_destroy(struct command_server* srv);

int push_new_command(int fd, const char *request);

#endif // _COMMAND_SERVER_H_
