#ifndef _NAME_GENERATOR_H_
#define _NAME_GENERATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
//#include <stdlib.h>

static inline void generate_new_object_name(char *name) {
   time_t rawtime;
   struct tm *info;
   time(&rawtime);
   info = localtime(&rawtime);
   char time_str[20];
   struct timeval tv;
   gettimeofday(&tv, 0);
   strftime(time_str, 20, "%F_%H-%M-%S", info);
   sprintf(name, "object_%s.%d", time_str, (int)tv.tv_usec);
   //free(info);
}

#ifdef __cplusplus
}
#endif

#endif //_NAME_GENERATOR_H_
