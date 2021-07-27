#include "FrameWrapper.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>

#include <numpy/arrayobject.h>

#include <opencv2/opencv.hpp>

#include "test.h"

void PyDestroyCvMat(PyObject* matCapsule)
{
    delete reinterpret_cast<cv::Mat*>(PyCapsule_GetPointer(matCapsule, NULL));
}

PyObject* cvMatToPyObject(cv::Mat mat)
{
    import_array();

    int dims = 2; //todo: get dims from matrix
    int depth = mat.depth();
    int cn = CV_MAT_CN(mat.type());
    auto size = mat.size();
    const int f = (int)(sizeof(size_t)/8);
    int typenum = depth == CV_8U ? NPY_UBYTE : depth == CV_8S ? NPY_BYTE :
                  depth == CV_16U ? NPY_USHORT : depth == CV_16S ? NPY_SHORT :
                  depth == CV_32S ? NPY_INT : depth == CV_32F ? NPY_FLOAT :
                  depth == CV_64F ? NPY_DOUBLE : f*NPY_ULONGLONG + (f^1)*NPY_UINT;

    npy_intp sizes[3];
    sizes[0] = size.height;
    sizes[1] = size.width;

    if( cn > 1 )
    {
        sizes[dims++] = cn;
    }

    float *p = mat.ptr<float>(0);
    std::cout << "Mat" << mat << std::endl;

    PyObject* matCapsule = PyCapsule_New(new cv::Mat(mat), NULL, PyDestroyCvMat);
    PyObject* ndarray = PyArray_SimpleNewFromData(dims, sizes, typenum, p);
    PyArray_SetBaseObject(reinterpret_cast<PyArrayObject*>(ndarray), matCapsule);
    return ndarray;
}

extern "C" PyObject* dclprocessor_image(PyObject* self, PyObject* args)
{
    printf("I'm in dclprocessor_image 0\n");
    PyObject *frameCapsule;
    if(!PyArg_ParseTuple(args, "O", &frameCapsule))
    {
        printf("aaaaAa");
        exit(0);
    }
    printf("I'm in dclprocessor_image 1\n");
    auto& frame = *reinterpret_cast<tmk::storage::Frame*>(PyCapsule_GetPointer(frameCapsule, NULL));
    auto image = frame.image();
    return cvMatToPyObject(image);
}
