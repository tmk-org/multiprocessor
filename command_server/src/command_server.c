#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#include "misc/ring_buffer.h"
#include "command_server/command_server.h"
#include "model/model.h"

//#include "api/api.h"

static struct model_desc *mdesc = NULL;

static struct ring_buffer *cmd_ring = NULL;

#if 1
int executeAPICommand(const char *cmd, int *exit_flag) {
    fprintf(stdout, "[command_server]: TODO real execute command by api\n");
    fflush(stdout);
    if (strcasecmp(cmd, "STOP\n") == 0 || strcasecmp(cmd, "S\n") == 0 ||
        strcasecmp(cmd, "STOP") == 0 || strcasecmp(cmd, "S") == 0) { 
        fprintf(stdout, "[command_server]: STOP command recieved\n");
        fflush(stdout);
        *exit_flag = 1;
        if (mdesc) model_destroy(mdesc);
        mdesc= NULL;
    }
    else if (strcasecmp(cmd, "INIT\n") == 0 || strcasecmp(cmd, "I\n") == 0 ||
             strcasecmp(cmd, "INIT") == 0 || strcasecmp(cmd, "I") == 0 ) { 
        fprintf(stdout, "[command_server]: INIT command recieved\n");
        fflush(stdout);
        mdesc = model_init(NULL);
        if (mdesc == NULL) {
            fprintf(stdout, "[command_server]: command '%s' failed\n", cmd);
            fflush(stdout);
        }
    } 
    else if (strcasecmp(cmd, "CONFIG\n") == 0 || strcasecmp(cmd, "C\n") == 0 ||
             strcasecmp(cmd, "CONFIG") == 0 || strcasecmp(cmd, "C") == 0) { 
        char fname[255];
        fprintf(stdout, "[command_server]: INIT command recieved\n");
        fflush(stdout);
        //waits on input is not good idea for any server solution
        //it waits IN the SERVER terminal and get answer too...
        //TODO: use api which can support parameters as filename with the path
        fprintf(stdout, "[command_server]: input model configuration filename\n");        
        scanf("%s", fname);
        mdesc = model_init(fname);
        if (mdesc == NULL) {
            fprintf(stdout, "[command_server]: command '%s' failed\n", cmd);
            fflush(stdout);
        }
        else {
            //TODO: make callback
            //we needed in callback on reply after init because this reply is not informative about real status
            fprintf(stdout, "[command_server]: starting model from %s...\n", cmd, fname);
            fflush(stdout);
        }
    } 
    else {
        fprintf(stdout, "[command_server]: Received unknown command '%s'\n", cmd);
        fflush(stdout);
        return -1;
    }
    return 0;
}
#endif

int push_new_command(int fd, const char *request) {
    //Create command and add into queue
    command_t *new_cmd = (command_t *)malloc(sizeof(command_t));
    memset(new_cmd, 0, sizeof(command_t));
    strcpy(new_cmd->full_command, request);
    size_t n = strlen(new_cmd->full_command - 1);
    if (new_cmd->full_command[n - 1] == '\n') new_cmd->full_command[n - 1] = '\0';
    new_cmd->fd = fd;
    fprintf(stdout, "[command_server]: put to queue new command %s\n", new_cmd->full_command);
    fflush(stdout);
    ring_buffer_push(cmd_ring, (size_t)new_cmd);
    return 0;
}

command_t *pop_next_command() {
    return (command_t*)ring_buffer_pop(cmd_ring);
}

void* command_thread(void *arg) {
    //pthread_mutex_lock(&stop_mutex);
    //??? exit_flag
    //pthread_mutex_unlock(&stop_mutex);
    struct command_server *cmdsrv = (struct command_server *)arg;
    command_t *cmd;
    char command[COMMAND_BUF_LEN];
    int rc;

    fprintf(stdout, "[command_server]: Starting command server\n");
    fflush(stdout);

    while (!(*(cmdsrv->exit_flag))) {
        cmd = pop_next_command();
        if (cmd == NULL) {
            sleep(1);
            continue;
        }
        fprintf(stdout, "[command_server]: Received command '%s'\n", cmd->full_command);
        fflush(stdout);
        //Execute command by API
        executeAPICommand(cmd->full_command, cmdsrv->exit_flag);
        if (cmd) free(cmd);
        cmd = NULL;
    }

    fprintf(stdout, "[command_server]: Stopping command server...\n");
    fflush(stdout);
    return NULL;
}

struct command_server *command_server_init(int *exit_flag){
    struct command_server *cmdsrv = (struct command_server *)malloc(sizeof(struct command_server));
    cmd_ring = ring_buffer_create(COMMAND_QUEUE_SIZE, 0);
    if (cmd_ring == NULL) {
        fprintf(stdout, "[command_server]: can't create ring buffer\n");
        fflush(stdout);
        *exit_flag = 1;
        free(cmdsrv);
        return NULL;
    }
    cmdsrv->exit_flag = exit_flag;
    pthread_create(&cmdsrv->thread_id, NULL, command_thread, (void *)cmdsrv);
    return cmdsrv;
}

int command_server_destroy(struct command_server *cmdsrv) {
    //TODO: fake command for this aim
    push_new_command(0, "STOP\n");
    //TODO: wait for finish
    pthread_join(cmdsrv->thread_id, NULL);
    ring_buffer_destroy(cmd_ring);
    free(cmdsrv);
    return 0;
}

