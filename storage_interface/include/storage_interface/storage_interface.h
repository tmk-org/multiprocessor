#ifndef _STORAGE_INTERFACE_H_
#define _STORAGE_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#define BOOST_SEGMENT_NAME_ENV	"BOOST_SEGMENT_NAME"

int storage_interface_init(char* segment_name, size_t size);
void storage_interface_new_object(char* segment_name, char *obj_name);
void storage_interface_free_object(char* segment_name, char *obj_name);
void storage_interface_destroy(char* segment_name);

#ifdef __cplusplus
}
#endif

#endif //_STORAGE_INTERFACE_H_
