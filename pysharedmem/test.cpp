#include "test.h"

#include <cstdio>
#include <cstdlib>

namespace tmk::storage
{

float data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

Frame::Frame() {
    std::cout << "created frame at " << this << std::endl;
}

cv::Mat Frame::image() {
    int ndims = 2;
    int sizes[2] = {5, 2};
    cv::Mat mat(ndims, sizes, CV_32F, data);
    std::cout << "created mat: " << mat << std::endl;
    return mat;
}

Frameset::Frameset() : Frameset(3) {
}

Frameset::Frameset(size_t framesCount) : frames(framesCount) {
    printf("created Frameset at %lu\n", this);
}

size_t Frameset::framesCount() {return frames.size();}

std::vector<FrameID> Frameset::framesIDs() {
    std::vector<FrameID> ids;
    for (int i = 0; i < 10; i++)
    {
        ids.push_back(FrameID("frame id = " + std::to_string(i)));
        ids.back().id = i;
    }

    return ids;
}

FrameRef Frameset::frame(const FrameID &id) {
    auto res =  FrameRef(frames[id.id]);
    std::cout << "Ref is " << &res << std::endl;
    return res;
}

ObjectDescriptor::ObjectDescriptor(size_t framesetsCount) : framesets(framesetsCount) {
    printf("created ObjectDescriptor at %lu\n", this);
}

ObjectDescriptor::~ObjectDescriptor() {}

size_t ObjectDescriptor::framesetsCount() {return framesets.size();}

std::vector<FramesetID> ObjectDescriptor::framesetsIDs() {
    std::vector<FramesetID> ids;
    for (int i = 0; i < 10; i++)
    {
        ids.push_back(FramesetID("frameset id = " + std::to_string(i)));
        ids.back().id = i;
    }

    return ids;
}

FramesetRef ObjectDescriptor::frameset(const FramesetID &id) {
    return FramesetRef(framesets[id.id]);
}

}

int pipeline_middle_process(execute_object_func_t exec_object) {
    tmk::storage::ObjectDescriptor objectDescriptor(42);
    const char* arg1 = "arg1";
    const char* arg2 = "arg2";
    printf("Hello, I'm in C code: pipeline_middle_process\n");
    auto res = exec_object(objectDescriptor, const_cast<char*>(arg1), const_cast<char*>(arg2));
    return res;
}

int test(int x) {
    printf("Do nothing, x = %d\n", x);
    return (x+5);
}
