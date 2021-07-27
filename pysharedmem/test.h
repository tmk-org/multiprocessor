#ifndef _TEST_H_
#define _TEST_H_

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

#include <opencv2/opencv.hpp>

int test(int x);

namespace tmk {
namespace storage {
template <typename ObjTy> struct ObjectRef {
  ObjectRef() = default;
  ObjectRef(ObjTy &f) : pObj(&f) {}
  operator bool() { return valid(); }
  bool valid() { return pObj != nullptr; }
  operator ObjTy&() { return get(); }
  ObjTy &operator()() { return get(); }
  ObjTy &get() {
    if (!valid()) {
      throw std::logic_error("Reference is not initialized");
    }
    return *pObj;
  }

protected:
  ObjTy *pObj = nullptr;
};

constexpr uint64_t IMPOSSIBLE_ID = 0;
constexpr uint64_t MAX_NAME_LENGTH = 71;

struct ObjectID {
  char name[MAX_NAME_LENGTH + 1];
  uint64_t id = IMPOSSIBLE_ID;

  ObjectID() { name[0] = 0; }
  ObjectID(const std::string &name_) {
    if (name_.length() > MAX_NAME_LENGTH) {
      std::cout << "ObjectID(): the length of string specified exceeds "
                      "maximal possible length "
                   << MAX_NAME_LENGTH << " (clipped)";
    }
    // data plus finishing zero
    name[name_.copy(name, MAX_NAME_LENGTH)] = 0;
  }

  ~ObjectID() = default;
};

struct FrameID : public ObjectID {
  using ObjectID::ObjectID;
};

class Frame {
public:
    Frame();
    ~Frame() = default;
    cv::Mat image();
};

using FrameRef = ObjectRef<Frame>;

struct FramesetID : public ObjectID {
  using ObjectID::ObjectID;
  ~FramesetID() = default;
};

class Frameset {
  public:
  Frameset();
  Frameset(size_t framesCount);
  ~Frameset() = default;

  size_t framesCount();

  std::vector<FrameID> framesIDs();

  FrameRef frame(const FrameID &id);

  private:
  std::vector<Frame> frames;
};

using FramesetRef = ObjectRef<Frameset>;

struct ObjectDescriptorID : public ObjectID {
  using ObjectID::ObjectID;
};

class ObjectDescriptor {
  public:
  ObjectDescriptor(size_t framesetsCount);
  ~ObjectDescriptor();

  size_t framesetsCount();

  std::vector<FramesetID> framesetsIDs();

  FramesetRef frameset(const FramesetID &id);

  private:
  std::vector<Frameset> framesets;
};
}
}


typedef int (*execute_object_func_t)(const tmk::storage::ObjectDescriptor&, char*, char*);
int pipeline_middle_process(execute_object_func_t exec_object);

#endif
