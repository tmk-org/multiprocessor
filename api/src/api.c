#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api/api.h"

const char* apiGenerateTestReply(enum APICmdType cmdType, enum APICmdName cmdName) {
    (void)cmdType;
    switch (cmdName) {
        case API_INIT: return "RE006{1,0:init}";
        case API_STATUS: return "RE006{2,0:ok}";
        case API_GRAB: return "RE027{3,0:api_20210303T191628_client0_1.pgm}";
        case API_LIVE: return "";
        case API_CONTROL: return "";
        case API_CLEAN: return "";
        case API_STOP: return "RE006{7,0:ok}";
        default: return "";
    }
    return "UNKNOWN";
}

const char *apiCmdStatus(enum APICmdStatus cmdStatus) {
   switch (cmdStatus) {
        case API_CMD_QUERY: return "API_QUERY";
        case API_CMD_WAIT_REPLY: return "API_WAIT_REPLY";
        case API_CMD_REPLY: return "API_REPLY";
        case API_CMD_OK: return "API_OK";
        case API_CMD_NO_REPLY: return "API_NO_REPLY";
        case API_CMD_UNKNOWN: return "API_UNKNOWN";
   }
   return "Unknown cmd status";
}

enum APICmdStatus apiCmdNeedReplay(enum APICmdType cmdType) {
    switch (cmdType) {
        case API_GET:
            return API_CMD_WAIT_REPLY;
        case API_POST:
            //return API_CMD_NO_REPLY;
            return API_CMD_WAIT_REPLY;
        case API_UNDEFINED:
            return API_CMD_NO_REPLY;
    }
    return API_CMD_UNKNOWN;
}

const char *apiGenerateCommandString(enum APICmdType cmdType, enum APICmdName cmdName, int clientId, const char* value, const char* additional_value){
    char buf[1024];
    size_t pos;
    pos = snprintf(buf, sizeof(buf), "%s,", apiCmdType(cmdType));
    pos += snprintf(buf + pos, sizeof(buf) - pos, "/api/");
    if (clientId > 0){
        pos += snprintf(buf + pos, sizeof(buf) - pos, "client/%d/", clientId);
    }
    else if (clientId == 0) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "server/");
    }
    if (cmdName != API_UNINITED) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s", apiCmdName(cmdName));
    }
    else if (value && strlen(value) > 0){
        pos += snprintf(buf + pos, sizeof(buf) - pos, value);
    }
    if (/*cmdType == API_POST &&*/ additional_value && strlen(additional_value) > 0) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, ",%s", additional_value);
    }
    return strdup(buf);
}

void apiFreeString(const char *string){
    free((void*)string);
}

void apiParseReply(struct api_reply *reply, const char *rstr){
    char status[3];
    char buf[0xFFF];
    int errCode = 0;
    sscanf(rstr, "%2s%x{%d,%[^}]}", status, &reply->msgLength, &reply->cmdName, buf);
    reply->replyStr = strdup(buf);
    if (strcmp(status, "OK") == 0){
        reply->replyResult = API_OK;
    }
    else {
        reply->replyResult = API_ERROR;
    }
    switch (reply->cmdName) {
        case API_GRAB: {
            char *res = reply->replyStr;
            sscanf(res, "%d:%s", &errCode, buf);
            apiFreeString(res);
            reply->replyStr = strdup(buf);
            break;
        }
        default:
            errCode = 0;
    }
    printf("apiParseReply() %s --> [RE%d,%s]:%s\n", rstr, reply->msgLength, apiCmdName(reply->cmdName), reply->replyStr);
}



