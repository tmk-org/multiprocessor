#ifndef _PIPELINE_H
#define _PIPELINE_H

struct pipeline;

//server part
struct pipeline *pipeline_init(char *init_string);
void pipeline_destroy(struct pipeline *pdesc);

//client part
typedef void (*pipeline_action_t) (void *data, int *status);

int pipeline_process(void (*pipeline_action) (void *data, int *status));

#endif //_PIPELINE_H
