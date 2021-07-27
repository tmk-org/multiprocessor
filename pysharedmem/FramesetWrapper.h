#ifndef FRAMESET_WRAPPER_H
#define FRAMESET_WRAPPER_H

#define PY_SSIZE_T_CLEAN
#include <Python.h> //python3-dev

constexpr char dclprocessor_framesCount_docs[] = "Frameset::framesCount() wrapper";
extern "C" PyObject* dclprocessor_framesCount(PyObject* self, PyObject* args);

constexpr char dclprocessor_framesIDs_docs[] = "Frameset::framesIDs() wrapper";
extern "C" PyObject* dclprocessor_framesIDs(PyObject* self, PyObject* args);

constexpr char dclprocessor_frame_docs[] = "Frameset::frame(const FrameID&) wrapper";
extern "C" PyObject* dclprocessor_frame(PyObject* self, PyObject* args);

#endif //FRAMESET_WRAPPER_H