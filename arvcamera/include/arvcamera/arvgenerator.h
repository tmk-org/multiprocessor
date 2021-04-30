#ifndef _ARV_GENERATOR_H_
#define _ARV_GENERATOR_H_

struct data_generator;
//TODO: what about host/port ???
struct data_generator *generator_init(char *interface, char *serial);
void generator_destroy(struct data_generator *generator);

#endif //_ARV_GENERATOR_H_
