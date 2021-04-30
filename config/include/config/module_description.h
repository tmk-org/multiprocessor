#ifndef _MODULE_DESCRIPTION_H_
#define _MODULE_DESCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

enum MT_TYPE {
    MT_UNDEFINED = -1,
    MT_FIRST = 1,
    MT_MIDDLE = 2,
    MT_LAST = 0
};

struct module_description{
    char **argv;  //command string for start module
    int id;       //idx of process in the process table
    enum MT_TYPE type; //for choose a type of the thread (source/middle/final)
    int next;     //idx of the next in the pipeline model
};

typedef struct module_description module_t;

#include <stdlib.h>

static inline void module_description_free(module_t *target){
    if (!target || !target->argv) return;
    for (int i = 0; target->argv[i] != NULL; i++) 
        free(target->argv[i]);
};

#ifdef __cplusplus
}
#endif


#endif //_MODULE_DESCRIPTION_H_
