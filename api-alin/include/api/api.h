//-----------------------------------------------------------------------
//  Created     : 08.04.21
//  Author      : alin
//  Description : Copied api from ../../api/
//-----------------------------------------------------------------------

#ifndef _API_H_
#define _API_H_

#ifdef __cplusplus
extern "C" {
#endif

enum APIResult {
    API_ERROR,
    API_OK
};

enum APICmdType {
    API_UNDEFINED = 0,
    API_GET,
    API_POST,
};

enum APICmdName {
    API_UNINITED = -1,
    API_INIT = 1,
    API_START,
    API_FINISH,
    API_STATUS,
    API_GRAB,
    API_LIVE,
    API_CONTROL,
    API_CLEAN,
    API_STOP
};

enum APICmdStatus{
    API_CMD_UNKNOWN = -1,
    API_CMD_QUERY,
    API_CMD_NO_REPLY,
    API_CMD_WAIT_REPLY,
    API_CMD_REPLY,
    API_CMD_OK
};

enum API_CLIENT_STATE{
    API_CLIENT_UNKNOWN = -1,
    API_CLIENT_OK,
    API_CLIENT_ERROR
};

enum API_CLIENT_ACTIVITY {
    API_ACT_UNKNOWN = -1,
    API_ACT_SLEEP,
    API_ACT_LIVE,
    API_ACT_GRAB
};

struct API_CLIENT {
    enum API_CLIENT_STATE status;
    int clientId;
};

struct API_CLIENTS {
    enum APIResult status;
    int clientsNumber;
    struct API_CLIENT *clients;
};

struct api_request {
    const char *requestStr;
};


struct api_reply {
    enum APIResult replyResult;
    int msgLength;
    enum APICmdName cmdName;
    char *replyStr;
};

static inline const char *apiCmdType(enum APICmdType cmdType){
    switch (cmdType) {
        case API_UNDEFINED: return "undefined";
        case API_GET: return "get";
        case API_POST: return "post";
    }
    return "UNKNOWN";
}

static inline const char *apiCmdName(enum APICmdName cmdName){
    switch (cmdName) {
        case API_UNINITED: return "uninited";
        case API_INIT: return "init";
        case API_START: return "start";
        case API_FINISH: return "finish";
        case API_STATUS: return "status";
        case API_GRAB: return "grab";
        case API_LIVE: return "live";
        case API_CONTROL: return "control";
        case API_CLEAN: return "clean";
        case API_STOP: return "stop";
    }
    return "unknown";
}

void apiParseReply(struct api_reply *reply, const char *rstr);
const char *apiGenerateCommandString(enum APICmdType cmdType, enum APICmdName cmdName, int clientId, char **values);

enum APICmdStatus apiCmdNeedReplay(enum APICmdType cmdType);
const char *apiCmdStatus(enum APICmdStatus cmdStatus);

#ifdef __cplusplus
}
#endif

#endif// _API_H_
