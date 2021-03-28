#include "storage_interface/storage_interface.h"

#include <gtest/gtest.h>

#include <sys/wait.h>

TEST(SharedMemory, ObjectSegmentCreateDeleteMemory) {
    char segment_name[80] = "SharedObjects";
    char obj_name[80] = "Object0";
    storage_interface_init(segment_name, 5013504*100);
    storage_interface_new_object(segment_name, obj_name);
    sleep(1);
    storage_interface_free_object(segment_name, obj_name);
    storage_interface_destroy(segment_name);
}

