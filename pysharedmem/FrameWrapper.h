#ifndef FRAME_WRAPPER_H
#define FRAME_WRAPPER_H

#define PY_SSIZE_T_CLEAN
#include <Python.h> //python3-dev

constexpr char dclprocessor_image_docs[] = "Frame::image() wrapper";
extern "C" PyObject* dclprocessor_image(PyObject* self, PyObject* args);

#endif //FRAME_WRAPPER_H
