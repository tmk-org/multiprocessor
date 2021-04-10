/* Json configuration file parser to "struct module_t[n]"
 * return value: < 0 if it failed, n if not
 * @autor Alin42, @date 26.03.2021
 */

#ifndef CONFIG_H 
#define CONFIG_H

#include "config/module_description.h" // struct module_t definition

#ifdef __cplusplus
extern "C"
#endif
//int model_read_configuration(const char *fileName, module_t **target);
int create_config(const char *fileName, module_t **target);

#endif

