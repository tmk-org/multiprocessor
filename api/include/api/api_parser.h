#ifndef _API_PARSER_H_
#define _API_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

char *execute_api_command_empty(const char *cmd, int *exit_flag);
char *executeAPICommand(const char *request, int *exit_flag);

#ifdef __cplusplus
}
#endif

#endif// _API_PARSER_H_
