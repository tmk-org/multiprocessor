#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "api/api.h"

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

const char *apiGenerateCommandString(enum APICmdType cmdType, enum APICmdName cmdName, int clientId, char** values) {
    char buf[1024];
    size_t pos;
    pos = snprintf(buf, sizeof(buf), "%s,", apiCmdType(cmdType));
    pos += snprintf(buf + pos, sizeof(buf) - pos, "/api/");
    if (clientId >= 0) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "client/%d/", clientId);
    }
    /*else if (clientId == 0) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "server/"); // 0︿0
    }*/
    if (cmdName != API_UNINITED) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s", apiCmdName(cmdName));
    }
    for (int i = 0; values && values[i] && strlen(values[i]) > 0; ++i) {
        pos += snprintf(buf + pos, sizeof(buf) - pos, ",%s", values[i]);
    }
    return strdup(buf);
}

void apiParseReply(struct api_reply *reply, const char *rstr) {
    char status[3];
    char buf[0xFFF];
    int errCode = 0;
    sscanf(rstr, "%2s%x{%d,%[^}]}", status, &reply->msgLength, &reply->cmdName, buf);
    reply->replyStr = strdup(buf);
    if (strcmp(status, "RE") == 0) {
        reply->replyResult = API_OK;
    }
    else {
        reply->replyResult = API_ERROR;
    }
    switch (reply->cmdName) {
        case API_GRAB: {
            char *res = reply->replyStr;
            sscanf(res, "%d,%s", &errCode, buf);
            free((void*)res);
            reply->replyStr = strdup(buf);
            break;
        }
        default:
            errCode = 0;
    }
    printf("apiParseReply() %s --> [RE%d,%s]:%s\n", rstr, reply->msgLength, apiCmdName(reply->cmdName), reply->replyStr);
}
