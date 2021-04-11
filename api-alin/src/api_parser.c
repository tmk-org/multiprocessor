/* THIS FILE SHOULDN'T BE USED */


#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <regex.h>

//#include "api/api_parser.h"
#ifndef _API_PARSER_H_
#define _API_PARSER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define API_FUNC(name) \
void api##name(const char *cmd, int *exit_flag); \
/*{ \
    fprintf(stdout, "[API]: Received unknown command '%s'\n", cmd); \
    fflush(stdout); \
    return; \
}*/

#include "api/api.inl"
#endif// _API_PARSER_H_
//#include "api/api_parser.h"
#include "api/api.h"

enum APICmdName currCmd = API_UNINITED;

const char *cmdRegTemplate = "^([^,]+),/api(/client/[1-9]){0,1}/([^,]+),{0,1}(.*){0,1}$"; 
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

    char status[3], answer[0xFFF], buf[0xFFF];

    //printf("[API] Request: %s\n", strndup(&request[parts[3].rm_so], parts[3].rm_eo - parts[3].rm_so));

#if 0
    #define API_SWITCH(name, NAME, type) \
    /* API_##NAME */ \
    if (strncmp(request + parts[3].rm_so, "##name##", strlen("##name##")) == 0) { \
        currCmd = API_##NAME; \
        if (clientId > 0) { \
            asprintf(&reply, "ER00C{%d,,unknown}", currCmd); \
            return reply; \
        } \
        else { \
            asprintf(&cmdStr, "scripts/API_##NAME##"); \
            #ifdef API_COMMANDS \
            api##name(cmdStr, exit_flag); \
            #endif \
        } \
    } else
    #include "api.inl"
#endif

    //API_INIT
    if (strncmp(request + parts[3].rm_so, "restart", strlen("restart")) == 0) {
        currCmd = API_INIT;
        if (clientId > 0) {
            asprintf(&reply, "ER00C{%d,,unknown}", currCmd);
            return reply;
        }
        else {
            asprintf(&cmdStr, "scripts/API_INIT");
            #ifdef API_COMANDS
            apiInit(cmdStr, exit_flag);
            #endif
        }
    }
    //API_FINISH
    else if (strncmp(request + parts[3].rm_so, "finish", strlen("finish")) == 0) {
        currCmd = API_FINISH;
        if (clientId > 0) {
            asprintf(&reply, "ER00C{%d,,unknown}", currCmd);
            return reply;
        }
        else {
            asprintf(&cmdStr, "scripts/API_FINISH");
            #ifdef API_COMANDS
            apiFinish(cmdStr, exit_flag);
            #endif
        }
    }
    //API_START
    else if (strncmp(request + parts[3].rm_so, "start", strlen("start")) == 0){
        currCmd = API_START;
        if (clientId > 0) {
            asprintf(&reply, "ER00C{%d,,unknown}", currCmd);
            return reply;
        }
        else {
            asprintf(&cmdStr, "scripts/API_START");
            #ifdef API_COMANDS
            apiStart(cmdStr, exit_flag);
            #endif
        }
    }
    //API_STATUS
    else if (strncmp(request + parts[3].rm_so, "status", strlen("status")) == 0){
        currCmd = API_STATUS;
        if (clientId > 0) {
            asprintf(&cmdStr, "scripts/API_STATUS %d", clientId);
            #ifdef API_COMANDS
            apiStatus(cmdStr, exit_flag);
            #endif
        }
        else {
            asprintf(&cmdStr, "scripts/API_STATUS");
            #ifdef API_COMANDS
            apiStatus(cmdStr, exit_flag);
            #endif
        }
    }
    //API_GRAB
     else if (strncmp(request + parts[3].rm_so, "grab", strlen("grab")) == 0) {
        //TODO: auto
        currCmd = API_GRAB;
        if (clientId > 0) {
            asprintf(&cmdStr, "scripts/API_GRAB %d", clientId);
            #ifdef API_COMANDS
            apiGrab(cmdStr, exit_flag);
            #endif
        }
        else {
            asprintf(&reply, "ER00C{%d,,unknown}", currCmd);
            return reply;
        }
    }
    //API_CONTROL
    else if (strncmp(request + parts[3].rm_so, "ctrl", strlen("ctrl")) == 0 ){
        currCmd = API_CONTROL;
        asprintf(&reply, "ER00C{%d,,unknown}", currCmd);
        return reply;        
    }    
    //API_CLEAN
    else if (strncmp(request + parts[3].rm_so, "clean", strlen("clean")) == 0) {
        currCmd = API_CLEAN;
        if (clientId > 0) {
            asprintf(&reply, "ER012{%d,,wrong command}", currCmd);
            return reply;
        }
        else {
            asprintf(&cmdStr, "scripts/API_CLEAN");
            #ifdef API_COMANDS
            apiClean(cmdStr, exit_flag);
            #endif
        }        
    }
    //API_LIVE
    else if (strncmp(request + parts[3].rm_so, "live", strlen("live")) == 0){
        currCmd = API_LIVE;
        asprintf(&reply, "ER017{%d,,wrong command live}", currCmd);
        return reply;
    }
    //API_STOP
    else if (strncmp(request + parts[3].rm_so, "stop", strlen("stop")) == 0) {
        currCmd = API_STOP;
        asprintf(&reply, "RE011{%d,,STOP command}", currCmd);
        #ifdef API_COMANDS
        apiStop(cmdStr, exit_flag);
        #endif
        *exit_flag = 1;
        return reply;
    }
    else {
        asprintf(&reply, "ER014{%d,,unknown command}", currCmd);
        return reply;
    }

    printf("Executed %s:\n", cmdStr);

    /*FILE *file = NULL;
    free(cmdStr);
    if (file == NULL) return strdup("ER018{0,,wrong 1st parameter}");

    while (!feof(file)) {
        answer[0] = '\0';
        buf[0]='\0';
        fgets(buf, sizeof(buf), file);
        printf("%s", buf);
        res = sscanf(buf, "%2[^:]:%s\n", status, answer);
        if (res < 2) continue;
        if (strcmp(status, "OK") != 0) continue;
        if (strcmp(status, "OK") == 0 && strlen(answer) > 0) {
            asprintf(&reply, "RE%03X{%d,%s}", (unsigned int)(4 + strlen(answer)), currCmd, answer);
            break;
        }
    }

    pclose(file);*/
    
    if (!reply) asprintf(&reply,"ER011{%d,,empty answer}", currCmd);

    return reply;
}
