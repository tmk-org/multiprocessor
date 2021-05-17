#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pipeline/pipeline.h"

void pipeline_action(void *data, int *status) {
    char s[80];
    static int data_cnt = 0;
    fprintf(stdout, "[first, pipeline_action]: Input data: ");
    fflush(stdout);
    scanf("%s", s);
    fflush(stdout);
    sprintf(data, "%d%s", data_cnt++, s);
    *status = 0;
}

int main(int argc, char *argv[]){
    pipeline_process(pipeline_action);
    return 0;
}
