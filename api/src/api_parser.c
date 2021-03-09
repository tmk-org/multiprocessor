#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#include <regex.h>

#include "api/api_parser.h"
#include "api/api.h"

enum APICmdName currCmd = API_UNINITED;

const char *cmdRegTemplate = "^([^,]+),/api/(/client/[1-9]){0,1}/([^,]+),{0,1}(.*){0,1}$"; 
regex_t cmdRegEx;
static int regInit = 0;

char *execute_api_command_empty(const char *cmd, int *exit_flag) {
    printf("[server]: TODO real execute command by api\n");
    if (strcasecmp(cmd, "STOP\n") == 0 || strcasecmp(cmd, "S\n") == 0) { 
        printf("[server]: STOP command recieved\n");
        *exit_flag = 1;
    }
    return strdup("OK\n");
}

char *executeAPICommand(const char *request, int *exit_flag) {
 	char *cmdStr, *reply = NULL;
	regmatch_t parts[5];
	printf("REQUEST: %s\n", request);
    if (regInit == 0) {
        int res = regcomp(&cmdRegEx, cmdRegTemplate, REG_EXTENDED);
	    if (res != 0) {
		    printf("Wrong template\n");
		    return 0;
	    }
	    regInit = 1;
	}
 	int res = regexec(&cmdRegEx, request, 5, parts, 0);
	if (res != 0) {
		return strdup("ER012{0,,wrong command}");
	}
	
    int clientId = (parts[2].rm_eo - parts[2].rm_so > 0) ? (request[parts[2].rm_so + strlen("/client/")] - '0') : 0;

#ifdef API_DEBUG
	int i;
	for (i = 0; i < 5; i++) {
    if (i == 2 && parts[i].rm_eo - parts[i].rm_so > 0) {
			 printf("clientId = %d\n", request[parts[i].rm_so + strlen("/client/")] - '0');
		}
		else if (i == 2) {
			printf("client = 0\n");
		}
		printf("p%d[%d, %d]: %.*s\n", i, parts[i].rm_so, parts[i].rm_eo, parts[i].rm_eo - parts[i].rm_so, request + parts[i].rm_so);
  }
#endif

    char status[3], answer[0xFFF], buf[0xFFF];

    printf("Request %s: ", request);

    //API_INIT
 	if (strncmp(request + parts[3].rm_so, "restart", strlen("restart")) == 0) {
        currCmd = API_INIT;
		if (clientId > 0) {
			asprintf(&reply, "ER00C{%d,,unknown}", currCmd);
			return reply;
		}
		else {
			asprintf(&cmdStr, "scripts/API_INIT");
		}
	}
    //API_STATUS
	else if (strncmp(request + parts[3].rm_so, "status", strlen("status")) == 0){
		currCmd = API_STATUS;
		if (clientId > 0) {
			asprintf(&cmdStr, "scripts/API_STATUS %d", clientId);
		}
		else {
			asprintf(&cmdStr, "scripts/API_STATUS");
		}
    }
	//API_GRAB
 	else if (strncmp(request + parts[3].rm_so, "grab", strlen("grab")) == 0) {
        //TODO: auto
        currCmd = API_GRAB;
		if (clientId > 0) {
			asprintf(&cmdStr, "scripts/API_GRAB %d", clientId);
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
		*exit_flag = 1;
		return reply;
	}
	else {
	    asprintf(&reply, "ER014{%d,,unknown command}", currCmd);
		return reply;
    }

    printf("Execute %s:\n", cmdStr);

	FILE *file = NULL;
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
	
    if (!reply) asprintf(&reply,"ER011{%d,,empty answer}", currCmd);

	pclose(file);

	return reply;
}



