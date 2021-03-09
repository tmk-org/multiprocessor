#ifndef _TCP_PROCESS_H_
#define _TCP_PROCESS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MESSAGE_LEN 512

typedef char *(*execute_command_func_t) (const char *command, int *exit_flag);

//int tcp_server_process(char *port);
//OR int tcp_server_process(char *port, execute_command_func_t exec_cmd);
int tcp_server_process(char *port, char *(*exec_cmd) (const char *, int *exit_flag));

int tcp_client_process(char* addr, char *port);
int tcp_client_with_select_process(char* addr, char *port);

#ifdef __cplusplus
}
#endif

#endif// _TCP_PROCESS_H_
