#ifndef OBJECT_DESCRIPTOR_WRAPPER_H
#define OBJECT_DESCRIPTOR_WRAPPER_H

#define PY_SSIZE_T_CLEAN
#include <Python.h> //python3-dev

constexpr char dclprocessor_framesetsCount_docs[] = "ObjectDescriptor::framesetsCount() wrapper";
extern "C" PyObject* dclprocessor_framesetsCount(PyObject* self, PyObject* args);

constexpr char dclprocessor_framesetsIDs_docs[] = "ObjectDescriptor::framesetsIDs() wrapper";
extern "C" PyObject* dclprocessor_framesetsIDs(PyObject* self, PyObject* args);

constexpr char dclprocessor_frameset_docs[] = "ObjectDescriptor::frameset(const FramesetID&) wrapper";
extern "C" PyObject* dclprocessor_frameset(PyObject* self, PyObject* args);

#endif //OBJECT_DESCRIPTOR_WRAPPER_H