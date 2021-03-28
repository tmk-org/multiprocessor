#include <storage/Ipc.h>
#include <storage/ObjectDescriptor.h>

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace tmk;
using namespace tmk::storage;

void testReadFrame() {
    auto [segment, clientShmemAlloc] = ipc::openSharedMemory("SharedObjects");

    const std::string name = "Object0";
    auto myObj = ipc::findShared<ObjectDescriptor>(segment, name);
#if 1
    std::cout << "1 " << myObj().framesetsCount() << std::endl;

    auto framesetID = myObj().framesetsIDs()[0];

    std::cout << "2 " << myObj().framesetsIDs()[0] << std::endl;
   
    auto myFrameset = myObj().frameset(framesetID);
    
    std::cout << "3 " << myFrameset().framesCount() << std::endl;
    
    auto frameID = myFrameset().framesIDs()[0];
    
    std::cout << "2 " << frameID << std::endl;
    
    auto myFrame = myFrameset().frame(frameID);
    
    auto img = myFrame().image();
#else 
    auto img = myObj().frameset(myObj().framesetsIDs()[0])().frame(myFrameset().framesIDs()[0])().image();
#endif

    int imgSize = img.total() * img.elemSize();
    std::cout << "Find frame withw imgSize: " << imgSize << std::endl; 
    cv::imwrite("test_res.bmp", img);
}

int main() {

    testReadFrame();

    return 0;
}
