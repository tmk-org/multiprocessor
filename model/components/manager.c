#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tcp/tcp_process.h"
#include "config/config.h"
#include "api/api.h"
#include "api/api_parser.h"

//namespace {
static module_t *configuration;
static int modules_count; // const
static int configuration_iterator;
//} // namespace


void parseReply(char *reply) {
    struct api_reply apiReply;
    apiParseReply(&apiReply, reply);
    //printf("%s", apiReply.replyStr);
}

void wait_for_cmd(int if_statement) {
    if (if_statement) {
        char *s;
        size_t s_size;
        fprintf(stdout, "Type anything to go to next stage: ");
        fflush(stdout);
        getline(&s, &s_size, stdin);
    }
}

const char *getRequest() {
    ++configuration_iterator;
    if (configuration_iterator < modules_count) {
        wait_for_cmd(configuration_iterator == 0); // stage start modules
        return apiGenerateCommandString(API_GET, API_INIT, 0, "xxx", "yyy"); // say start module #i
    } else if (configuration_iterator - modules_count < modules_count) {
        wait_for_cmd(configuration_iterator == modules_count); // stage stop modules
        return apiGenerateCommandString(API_GET, API_CLEAN, 0, "xxx", "yyy"); // say stop module #i
    } else if (configuration_iterator - modules_count == modules_count) {
        wait_for_cmd(1); // wait anyway, stage say goodbye
        return apiGenerateCommandString(API_GET, API_STOP, 0, "xxx", "yyy"); // say goodbye
    } else {
        exit(0); // dangerous, TODO better way
    }
}

int main(int argc, char *argv[]) {
    
    if (argc != 4) {
	    fprintf(stderr, "Usage: %s host port config_file\n", argv[0]);
		exit(EXIT_FAILURE);
    }
    modules_count = create_config(argv[3], &configuration); // init const modules_count
    configuration_iterator = -1;
    if (modules_count < 1) {
        fprintf(stderr, "Configuration %s failed\n", argv[3]);
        exit(EXIT_FAILURE);
    }

    /* both clients (should?) use these handlers */
#if defined NONBLOCK_INPUT
    tcp_client_with_select_process(argv[1], argv[2]); // , parseReply, getRequest);
#else
    tcp_client_process(argv[1], argv[2]); // , parseReply, getRequest);
#endif

     /* TODO add operator delete for "configuration var" from c++, NOT func free() */

    return 0;
}
