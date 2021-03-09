#include <stdio.h>

#include "api/api.h"
#include "api/api_parser.h"

int main(){
    int exitFlag = 0, clientId = 0, cmdType = API_GET, cmdName = API_GRAB;
    const char *request = apiGenerateCommandString(API_GET, API_GRAB, clientId, "xxx", "yyy");
    printf("Request: %s\n", request);
    //TODO: OK or ERROR reply for all commands
    printf("Test reply: %s\n", apiGenerateTestReply(cmdType, cmdName));
    printf("Start executor...\n");
    //TODO: executor with callback
    printf("Reply: %s\n", executeAPICommand(request, &exitFlag));
    return 0;
}
