#ifndef _MODEL_H
#define _MODEL_H

struct model_desc;

struct model_desc *model_init(char *fname);
void model_destroy(struct model_desc *mdesc);

#endif //_MODEL_H
