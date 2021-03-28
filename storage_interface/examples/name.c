#include <stdio.h>

#include "storage_interface/name_generator.h"

int main(){
    char name[80];
    generate_new_object_name(name);
    printf("%s\n", name);
    return 0;
}