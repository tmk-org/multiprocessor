#include "storage_interface/storage_interface.h"

#include <storage/Ipc.h>
#include <storage/ObjectDescriptor.h>

#include <stdio.h>

using namespace tmk;
using namespace tmk::storage;

extern "C" int storage_interface_init(char* segment_name, size_t size){
    //TODO: think about NULL
    // Create boost shared memory
    ipc::createSharedMemory(segment_name, size);
    if (setenv(BOOST_SEGMENT_NAME_ENV, segment_name, 1) != 0){
	    fprintf(stderr, "[storage_interface]: can't set %s value for child process: errno=%d, %s\n", 
		    BOOST_SEGMENT_NAME_ENV, errno, strerror(errno));
		return -1;
    }
    return 0;
}

extern "C" void storage_interface_new_object(char* segment_name, char *obj_name) {
    // Connect to shared as client and create object with name
    auto [segment, clientShmemAlloc] = ipc::openSharedMemory(segment_name);
    ObjectRef<ObjectDescriptor> object = ipc::createShared<ObjectDescriptor>(
      segment, obj_name, StorageType::NO_SAVE, ObservableType::PIPE);
}

extern "C"  void storage_interface_free_object(char* segment_name, char *obj_name) {
    // Connect to shared as client and destroy object by name
    auto [segment, clientShmemAlloc] = ipc::openSharedMemory(segment_name);
    ipc::destroyShared<ObjectDescriptor>(segment, obj_name);
}

extern "C"  void storage_interface_destroy(char* segment_name) {
#if 0
    struct shm_remove {
        shm_remove() { ipc::destroySharedMemory(segment_name); }
        ~shm_remove() { ipc::destroySharedMemory(segment_name); }
    } remover;
#else
    #include <sys/mman.h>
    shm_unlink(segment_name);
#endif
}

