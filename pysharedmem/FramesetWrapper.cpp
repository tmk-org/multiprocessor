#include "FramesetWrapper.h"

#include <Python.h>
#include "test.h"

extern "C" void PyFrameIDDestructor(PyObject *idCapsule)
{
    delete reinterpret_cast<tmk::storage::FrameID*>(PyCapsule_GetPointer(idCapsule, NULL));
}

extern "C" PyObject* dclprocessor_framesCount(PyObject* self, PyObject* args)
{
    printf("I'm in dclprocessor_framesCount 0\n");
    PyObject* framesetCapsule;
    if(!PyArg_ParseTuple(args, "O", &framesetCapsule))
    {
        printf("aaaaAa");
        exit(0);
    }
    printf("I'm in dclprocessor_framesCount 1\n");
    tmk::storage::Frameset& frameset = *reinterpret_cast<tmk::storage::Frameset*>(PyCapsule_GetPointer(framesetCapsule, NULL));
    return Py_BuildValue("i", frameset.framesCount());
}

extern "C" PyObject* dclprocessor_framesIDs(PyObject* self, PyObject* args)
{
//std::vector<FramesetID> framesIDs()
    printf("I'm in dclprocessor_framesIDs 0\n");
    PyObject* framesetCapsule;
    if(!PyArg_ParseTuple(args, "O", &framesetCapsule))
    {
        printf("aaaaAa");
        exit(0);
    }
    printf("I'm in dclprocessor_framesIDs 1\n");
    tmk::storage::Frameset& frameset = *reinterpret_cast<tmk::storage::Frameset*>(PyCapsule_GetPointer(framesetCapsule, NULL));
    auto ids = frameset.framesIDs();
    PyObject* resultList = PyList_New(0);
    for (const auto& id : ids)
    {
        auto new_id = new tmk::storage::FrameID(id);
        PyList_Append(resultList, PyCapsule_New(new_id, NULL, PyFrameIDDestructor));
    }
    return resultList;
}

extern "C" PyObject* dclprocessor_frame(PyObject* self, PyObject* args)
{
 //FramesetRef frame(const FramesetID &id)
    printf("I'm in dclprocessor_frame 0\n");
    PyObject *framesetCapsule, *idCapsule;
    if(!PyArg_ParseTuple(args, "OO", &framesetCapsule, &idCapsule))
    {
        printf("aaaaAa");
        exit(0);
    }
    printf("I'm in dclprocessor_frame 1\n");
    tmk::storage::Frameset& frameset = *reinterpret_cast<tmk::storage::Frameset*>(PyCapsule_GetPointer(framesetCapsule, NULL));
    tmk::storage::FrameID& id = *reinterpret_cast<tmk::storage::FrameID*>(PyCapsule_GetPointer(idCapsule, NULL));
    auto frameRef = frameset.frame(id);
    return PyCapsule_New(reinterpret_cast<void*>(&frameRef), NULL, NULL);
}

