#include <storage/Ipc.h>
#include <storage/ObjectDescriptor.h>

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace tmk;
using namespace tmk::storage;

int main() {

    auto [segment, clientShmemAlloc] = ipc::openSharedMemory("SharedObjects");
    
    std::string name = "Object0";
    auto clientObj = ipc::findShared<ObjectDescriptor>(segment, name);

    std::cout << "Constructing Mat" << std::endl;

    TimeT tstamp = system_clock::now();
    int sequenceNum = 0;
    int source = 0;
    StorageType st = StorageType::NO_SAVE;
    cv::Mat img = cv::imread("test.bmp", cv::IMREAD_ANYDEPTH |
                                                  cv::IMREAD_ANYCOLOR);
    int imgSize = img.total() * img.elemSize();

    std::cout << "Constructing Frame with size " << imgSize << std::endl;
    int ret = 0;

    try {
      auto frameset = clientObj().addFrameset(clientShmemAlloc, "Frameset0",
                                              StorageType::NO_SAVE);
      frameset().addFrame(clientShmemAlloc, "frame0", StorageType::NO_SAVE, img,
                          tstamp, sequenceNum++,
                          source);
      frameset().addFrame(clientShmemAlloc, "frame1", StorageType::NO_SAVE, img,
                          tstamp, sequenceNum++,
                          source);

      std::cout << "Frames count: " << frameset().framesCount() << std::endl;
    } 
    catch (const std::exception &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
