#include "ObjectDescriptorWrapper.h"

#include <Python.h>
#include "test.h"

extern "C" void PyFramesetIDDestructor(PyObject *idCapsule)
{
    delete reinterpret_cast<tmk::storage::FramesetID*>(PyCapsule_GetPointer(idCapsule, NULL));
}

extern "C" PyObject* dclprocessor_framesetsCount(PyObject* self, PyObject* args)
{
    printf("I'm in dclprocessor_framesetsCount 0\n");
    PyObject* objectDescriptorCapsule;
    if(!PyArg_ParseTuple(args, "O", &objectDescriptorCapsule))
    {
        printf("aaaaAa");
        exit(0);
    }
    printf("I'm in dclprocessor_framesetsCount 1\n");
    tmk::storage::ObjectDescriptor& objectDescriptor = *reinterpret_cast<tmk::storage::ObjectDescriptor*>(PyCapsule_GetPointer(objectDescriptorCapsule, NULL));
    return Py_BuildValue("i", objectDescriptor.framesetsCount());
}

extern "C" PyObject* dclprocessor_framesetsIDs(PyObject* self, PyObject* args)
{
//std::vector<FramesetID> framesetsIDs()
    printf("I'm in dclprocessor_framesetsIDs 0\n");
    PyObject* objectDescriptorCapsule;
    if(!PyArg_ParseTuple(args, "O", &objectDescriptorCapsule))
    {
        printf("aaaaAa");
        exit(0);
    }
    printf("I'm in dclprocessor_framesetsIDs 1\n");
    tmk::storage::ObjectDescriptor& objectDescriptor = *reinterpret_cast<tmk::storage::ObjectDescriptor*>(PyCapsule_GetPointer(objectDescriptorCapsule, NULL));
    auto ids = objectDescriptor.framesetsIDs();
    PyObject* resultList = PyList_New(0);
    for (const auto& id : ids)
    {
        auto new_id = new tmk::storage::FramesetID(id);
        PyList_Append(resultList, PyCapsule_New(new_id, NULL, PyFramesetIDDestructor));
    }
    return resultList;
}

extern "C" PyObject* dclprocessor_frameset(PyObject* self, PyObject* args)
{
 //FramesetRef frameset(const FramesetID &id)
    printf("I'm in dclprocessor_frameset 0\n");
    PyObject *objectDescriptorCapsule, *idCapsule;
    if(!PyArg_ParseTuple(args, "OO", &objectDescriptorCapsule, &idCapsule))
    {
        printf("aaaaAa");
        exit(0);
    }
    printf("I'm in dclprocessor_frameset 1\n");
    tmk::storage::ObjectDescriptor& objectDescriptor = *reinterpret_cast<tmk::storage::ObjectDescriptor*>(PyCapsule_GetPointer(objectDescriptorCapsule, NULL));
    tmk::storage::FramesetID& id = *reinterpret_cast<tmk::storage::FramesetID*>(PyCapsule_GetPointer(idCapsule, NULL));
    auto framesetRef = objectDescriptor.frameset(id);
    return PyCapsule_New(reinterpret_cast<void*>(&framesetRef.get()), NULL, NULL);
}

