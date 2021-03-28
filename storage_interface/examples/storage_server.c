#include <stdio.h>
#include "storage_interface/storage_interface.h"

int main(){
    char segment_name[80] = "SharedObjects";
    char obj_name[80] = "Object0";
    storage_interface_init(segment_name, 5013504*4);
    storage_interface_new_object(segment_name, obj_name);
    char c;
    scanf("%c", &c);
    storage_interface_free_object(segment_name, obj_name);
    storage_interface_destroy(segment_name);
    return 0;
}