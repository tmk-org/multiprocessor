#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
//-----------------------------------------------------------------------
//  Created     : 08.04.21
//  Author      : alin
//  Description : Dynamic API parser (generates from api.inl)
//-----------------------------------------------------------------------

#ifndef _API_PARSER_H_
#define _API_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define API_FUNC(name) \
void api##name(const char *cmd, int *exit_flag); \
/*{ \
    fprintf(stdout, "[API]: Received unknown command '%s'", cmd); \
    fflush(stdout); \
    return; \
}*/

#include "api.inl"

/* This thing should generate the following:
void apiUninited(const char *cmd, int *exit_flag);
void apiInit(const char *cmd, int *exit_flag);
void apiStart(const char *cmd, int *exit_flag);
void apiFinish(const char *cmd, int *exit_flag);
void apiStatus(const char *cmd, int *exit_flag);
void apiGrab(const char *cmd, int *exit_flag);
void apiLive(const char *cmd, int *exit_flag);
void apiControl(const char *cmd, int *exit_flag);
void apiClean(const char *cmd, int *exit_flag);
void apiStop(const char *cmd, int *exit_flag);*/


//-----------------------------------------------------------------------
//  Created     : 08.04.21
//  Author      : Alin42
//  Description : api_parser.c should parse commands and execute them
//                since it's all about macros now, it was moved here
//-----------------------------------------------------------------------


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <regex.h>

#include "api/api.h"

enum APICmdName currCmd = API_UNINITED;

const char *cmdRegTemplate = "^([^,]+),/api(/client/[0-9]){0,1}/([^,]+),{0,1}(.*){0,1}$"; 
regex_t cmdRegEx;
static int regInit = 0;

char *executeAPICommand(const char *request, int *exit_flag) {
    char *cmdStr, *reply = NULL;
    printf("[API] REQUEST: %s\n", request);
    if (regInit == 0) {
        int res = regcomp(&cmdRegEx, cmdRegTemplate, REG_EXTENDED);
        if (res != 0) {
            printf("Wrong template\n");
            return 0;
        }
        regInit = 1;
    }
    regmatch_t parts[5]; // cmdRegEx.re_nsub
    int res = regexec(&cmdRegEx, request, 5, parts, 0);
    if (res != 0) {
        return strdup("ER012{0,,wrong command}");
    }
    
    int clientId = (parts[2].rm_eo - parts[2].rm_so > 0) ? (request[parts[2].rm_so + strlen("/client/")] - '0') : 0;
    //printf("[API] Request: %s\n", strndup(&request[parts[3].rm_so], parts[3].rm_eo - parts[3].rm_so));

    #define API_SWITCH__API_SWITCH_TYPE_OK_OK(name, Name, NAME)               \
    asprintf(&cmdStr, "scripts/API_"#NAME);                                   \
    api##Name(cmdStr, exit_flag);
    
    #define API_SWITCH__API_SWITCH_TYPE_OK_UNKNOWN(name, Name, NAME)          \
    if (clientId > 0) {                                                       \
        asprintf(&cmdStr, "scripts/API_"#NAME);                               \
        api##Name(cmdStr, exit_flag);                                         \
    } else {                                                                  \
        asprintf(&reply, "ER00C{%d,,unknown}", currCmd);                      \
        return reply;                                                         \
    }
    
    #define API_SWITCH__API_SWITCH_TYPE_UNKNOWN_OK(name, Name, NAME)          \
    if (clientId > 0) {                                                       \
        asprintf(&reply, "ER00C{%d,,unknown}", currCmd);                      \
        return reply;                                                         \
    }                                                                         \
    else {                                                                    \
         asprintf(&cmdStr, "scripts/API_"#NAME);                              \
         api##Name(cmdStr, exit_flag);                                        \
    }                                                                         \
    
    #define API_SWITCH__API_SWITCH_TYPE_UNKNOWN_UNKNOWN(name, Name, NAME)     \
    asprintf(&reply, "ER017{%d,,wrong command}", currCmd);                    \
    return reply;

    #define API_SWITCH(name, Name, NAME, type)                                \
    /* API_##NAME */                                                          \
    if (strncmp(request + parts[3].rm_so, #name, strlen(#name)) == 0) {       \
        currCmd = API_##NAME;                                                 \
        API_SWITCH##type(name, Name, NAME)                                    \
    } else
    #include "api.inl"
    {
        asprintf(&reply, "ER014{%d,,unknown command}", currCmd);
        return reply;
    }

    printf("Executed %s:\n", cmdStr);
    
    if (!reply) asprintf(&reply,"ER011{%d,,empty answer}", currCmd);

    return reply;
}

#ifdef __cplusplus
}
#endif

#endif// _API_PARSER_H_
