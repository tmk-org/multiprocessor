#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pipeline/pipeline.h"

void pipeline_action(void *data, int *status) {
    static int data_cnt = 0;
    fprintf(stdout, "[middle, pipeline_action]: Get: %s\n", (char*)data);
    fflush(stdout);
    *status = (strchr(data, '*') == NULL);
}

int main(int argc, char *argv[]){
    pipeline_process(pipeline_action);
    return 0;
}
