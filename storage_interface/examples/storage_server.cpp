#include <storage/Ipc.h>
#include <storage/ObjectDescriptor.h>
#include <iostream>

using namespace tmk;
using namespace tmk::storage;

int main(int argc, char *argv[]) {

    // Remove shared memory on construction and destruction
    struct shm_remove {
        shm_remove() { ipc::destroySharedMemory("SharedObjects"); }
        ~shm_remove() { ipc::destroySharedMemory("SharedObjects"); }
    } remover;

    // Create shared memory
    auto [segment, shmemAlloc] =
        ipc::createSharedMemory("SharedObjects", 5013504*4);  //enough to 4 images 2448*2048*8bit

    
    const std::string name = "Object0"; //pipe, mandel, billet

    ObjectRef<ObjectDescriptor> object = ipc::createShared<ObjectDescriptor>(
      segment, name, StorageType::NO_SAVE, ObservableType::PIPE);

    ObjectRef<ObjectDescriptor> clientObj = ipc::findShared<ObjectDescriptor>(segment, name);

    int a = 0;   
    std::cin >> a;

    if (object) {
        ipc::destroyShared<ObjectDescriptor>(segment, object);
    }

    return 0;
}

