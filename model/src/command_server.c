#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>

#include "misc/ring_buffer.h"
#include "model/command_server.h"
#include "model/model.h"

//#include "api/api.h"

static struct model_desc *mdesc = NULL;

int executeAPICommand(const char *cmd, int *exit_flag) {
    fprintf(stdout, "[command_server]: TODO real execute command by api\n");
    fflush(stdout);
    if (strcasecmp(cmd, "STOP\n") == 0 || strcasecmp(cmd, "S\n") == 0) { 
        fprintf(stdout, "[command_server]: STOP command recieved\n");
        fflush(stdout);
        *exit_flag = 1;
        if (mdesc) model_destroy(mdesc);
        mdesc= NULL;
    }
    else if (strcasecmp(cmd, "INIT\n") == 0 || strcasecmp(cmd, "I\n") == 0) { 
        fprintf(stdout, "[command_server]: INIT command recieved\n");
        fflush(stdout);
        mdesc = model_init(NULL);
        if (mdesc == NULL) {
            fprintf(stdout, "[command_server]: command '%s' failed\n", cmd);
            fflush(stdout);
        }
    } 
    else if (strcasecmp(cmd, "CONFIG\n") == 0 || strcasecmp(cmd, "C\n") == 0) { 
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

static struct ring_buffer *cmd_ring = NULL;
static pthread_mutex_t stop_mutex;
static pthread_t command_thread;
static sem_t command_ready;

void send_reply(command_t *cmd, int rc) {
    //TODO: Reply by API
    static const char ok_tmpl[] = "OK\n";
    static const char fail_tmpl[] = "ER %d\n";
    char buf[20];
    if (rc == 0) {
        sprintf(buf, ok_tmpl);
    }
    else {
        sprintf(buf, fail_tmpl, rc);
    }
    //send(cmd->fd, buf, strlen(buf), 0);
}

void send_reply_with_data(command_t *cmd, int success, int rc) {
   //TODO: Reply by API
   static const char ok_tmpl[] = "OK %d\n";
   static const char fail_tmpl[] = "ER %d\n";
   char buf[20];
   if (success != 0) {
       sprintf(buf, ok_tmpl, rc);
   }
   else {
       sprintf(buf, fail_tmpl, rc);
   }
   //send(cmd->fd, buf, strlen(buf), 0);
}

command_t* get_next_command() {
    sem_wait(&command_ready);
    return (command_t*)ring_buffer_pop(cmd_ring);
}

int add_command(command_t *cmd){
    ring_buffer_push(cmd_ring, (size_t)cmd);
    sem_post(&command_ready);
    return 0;
}

void* command_server(void *arg) {
    //pthread_mutex_lock(&stop_mutex);
    //??? exit_flag
    //pthread_mutex_unlock(&stop_mutex);
    int *exit_flag = (int *)arg;
    command_t *cmd;
    char command[COMMAND_BUF_LEN];
    int rc;
    int reply_with_data = 0;

    fprintf(stdout, "[command_server]: Starting command server\n");
    fflush(stdout);

    while (!(*exit_flag)) {
        cmd = get_next_command();
        if (cmd == NULL) {
            sleep(1);
            continue;
        }
        fprintf(stdout, "[command_server]: Received command '%s'\n", cmd->full_command);
        fflush(stdout);
        //Execute command by API
        executeAPICommand(cmd->full_command, exit_flag); ///????
        if (reply_with_data == 0) {
          send_reply(cmd, rc);
        }
        else {
          send_reply_with_data(cmd, (rc < 0)?0:1, rc);
        }
        if (cmd) free(cmd);
        cmd = NULL;
    }

    fprintf(stdout, "[command_server]: Stopping command server...\n");
    fflush(stdout);
    return NULL;
}

int command_server_init(int *exit_flag){
    cmd_ring = ring_buffer_create(COMMAND_QUEUE_SIZE, 0);
    if (cmd_ring == NULL) {
        fprintf(stdout, "[command_server]: Stopping command server...\n");
        fflush(stdout);
        *exit_flag = 1;
        return -1;
    }
    return pthread_create(&command_thread, NULL, command_server, (void *)exit_flag);
}

int command_server_destroy() {
    //TODO: fake command for this aim
    command_t *new_cmd = (command_t *)malloc(sizeof(command_t));
    memset(new_cmd, 0, sizeof(command_t));;
    strcpy(new_cmd->full_command, "STOP\n");
    add_command(new_cmd);
    pthread_join(command_thread, NULL);
    ring_buffer_destroy(cmd_ring);
    return 0;
}

