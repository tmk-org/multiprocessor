#ifndef _CV_GENERATOR_H_
#define _CV_GENERATOR_H_

struct data_generator;
struct data_generator *generator_init(char *port);
void generator_destroy(struct data_generator *generator);

#endif //_CV_GENERATOR_H_
