//https://pavel-karateev.gitbook.io/intermediate-python/raznoe/python_c_extension
//https://docs.python.org/3/extending/extending.html#a-simple-example
//https://cpp.hotexamples.com/examples/-/-/PyObject_CallMethod/cpp-pyobject_callmethod-function-examples.html

#define PY_SSIZE_T_CLEAN
#include <Python.h> //python3-dev

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/ndarrayobject.h>

#include "FrameWrapper.h"
#include "FramesetWrapper.h"
#include "ObjectDescriptorWrapper.h"

#include "test.h"

extern "C" PyObject* dclprocessor_sum(PyObject* self, PyObject* args){

    PyObject *listObj;

    // Входящие аргументы находятся в кортеже
    // В нашем случае есть только один аргумент - список, на который мы будем
    // ссылаться как listObj
    if (!PyArg_ParseTuple(args, "O", &listObj))
        return NULL;

    // Длина списка
    long length = PyList_Size(listObj);

    // Проходимся по всем элементам
    long i, sum =0;
    for(i = 0; i < length; i++){
        // Получаем элемент из списка - он также Python-объект
        PyObject* temp = PyList_GetItem(listObj, i);
        // Мы знаем, что элемент это целое число - приводим его к типу C long
        long elem = PyLong_AsLong(temp);
        sum += elem;
    }

    // Возвращаемое в Python-код значение также Python-объект
    // Приводим C long к Python integer
    return Py_BuildValue("i", sum);
}

extern "C" PyObject* dclprocessor_test(PyObject* self, PyObject* args) {
    int n;
    if (!PyArg_ParseTuple(args, "i", &n)) // Проверка и анализ аргументов, полученных функцией, на возможность использования в C
        return NULL;
    return Py_BuildValue("i", test(n)); // Получаем результат из C, упаковываем его вместе с PyObject и возвращаем
}

//https://www.oreilly.com/library/view/python-cookbook/0596001673/ch16s07.html
//??? extern void pipeline_middle_process(int (*)(const void *, int *));

static PyObject *py_callback_func = NULL;

static int
stub_callback_func(const tmk::storage::ObjectDescriptor &object, char *boost_path, char *obj_name)
{

    printf("I'm in stub_callback_func 0\n");

    // Build up the argument list...
    PyObject *arglist = PyTuple_New(3);
    PyObject *obj = PyCapsule_New(const_cast<void*>(reinterpret_cast<const void*>(&object)), NULL, NULL);
    PyObject *boost_path_py = Py_BuildValue("s", boost_path);
    PyObject *obj_name_py = Py_BuildValue("s", obj_name);
    PyTuple_SetItem(arglist, 0, obj);
    PyTuple_SetItem(arglist, 1, boost_path_py);
    PyTuple_SetItem(arglist, 2, obj_name_py);
    printf("I'm in stub_callback_func 1\n");


    printf("Args: %lu %s %s\n", reinterpret_cast<uintptr_t>(&object), boost_path, obj_name);
    int retvalue = -1;
#if 0
    // ...for calling the Python compare function

    PyObject *result = PyEval_CallObject(py_callback_func, arglist);

    if (result && PySet_Check(result)) {
        retvalue = PyLong_AsLong(result);
    }

    Py_XDECREF(result);
#else
    PyObject *result = PyEval_CallObject(py_callback_func, arglist);
    printf("result=%lu\n", result);
    if (result) {
        retvalue = PyLong_AsLong(result);
    }
    Py_XDECREF(result);
#endif
    //Py_DECREF(obj_name_py);
    //Py_DECREF(boost_path_py);
    //Py_XDECREF(obj);
    Py_XDECREF(arglist);
    printf("I'm in stub_callback_func 2\n");
    printf("retval = %i\n", retvalue);
    printf("I'm in stub_callback_func 3\n");
    return retvalue;
}

extern "C" PyObject* dclprocessor_pipeline_middle_process(PyObject* self, PyObject* args) {

    PyObject *pycallobj;
    if (!PyArg_ParseTuple(args, "O", &pycallobj))
        return NULL;

    // Make sure second argument is a function
    if (!PyCallable_Check(pycallobj)) {
        PyErr_SetString(PyExc_TypeError, "[PY_wrapper]: Need a callable object");
    } else {
        // Save the compare function. This obviously won't work for multithreaded
        // programs and is not even a reentrant, alas -- qsort's fault!
        py_callback_func = pycallobj;
        printf("callback addr=%lu\n", pycallobj);
        auto res = pipeline_middle_process(stub_callback_func);
        printf("I'm in dclprocessor_pipeline_middle_process 0\n");
        Py_INCREF(Py_None);
#if 0 //TODO: this is only from example
        if (PyList_Check(list)) {
            int size = PyList_Size(list);
            int i;

            // Make an array of (PyObject *), because qsort does not know about
            // the PyList object
            PyObject **v = (PyObject **) malloc( sizeof(PyObject *) * size );
            for (i=0; i<size; ++i) {
                v[i] = PyList_GetItem(list, i);
                // Increment the reference count, because setting the list 
                // items below will decrement the reference count
                Py_INCREF(v[i]);
            }
            qsort(v, size, sizeof(PyObject*), stub_compare_func);
            //TODO: this is example for list, we need to work with object
            for (i=0; i<size; ++i) {
                PyList_SetItem(list, i, v[i]);
                // need not do Py_DECREF - see above
            }
            free(v);
        }
#endif
    }
    printf("I'm in dclprocessor_pipeline_middle_process 1\n");
    Py_INCREF(Py_None);
    printf("I'm in dclprocessor_pipeline_middle_process 2\n");
    return Py_None;
}

// Немного документации
static char dclprocessor_sum_docs[] = "sum( ): added all elements of the list\n";
static char dclprocessor_test_docs[] = "test(): nothing, only test\n";
static char dclprocessor_pipeline_middle_process_docs[] = "pipeline_middle_process(WorkWithObject): WorkWithObject is function with parameters ObjectDescriptor& obj, char *boost_path, char* obj_name\n";

/*
Эта таблица содержит необходимую информацию о функциях модуля
<имя функции в модуле Python>, <фактическая функция>,
<ожидаемые типы аргументов функции>, <документация функции>
*/
static PyMethodDef dclprocessor_funcs[] = {
    // free funcs
    {"sum", (PyCFunction)dclprocessor_sum, METH_VARARGS, dclprocessor_sum_docs},
    {"test", (PyCFunction)dclprocessor_test, METH_VARARGS, dclprocessor_test_docs},
    {"pipeline_middle_process", (PyCFunction)dclprocessor_pipeline_middle_process, METH_VARARGS, dclprocessor_pipeline_middle_process_docs},

    // ObjectDesctiptor
    {"framesetsCount", (PyCFunction)dclprocessor_framesetsCount, METH_VARARGS, dclprocessor_framesetsCount_docs},
    {"framesetsIDs", (PyCFunction)dclprocessor_framesetsIDs, METH_VARARGS, dclprocessor_framesetsIDs_docs},
    {"frameset", (PyCFunction)dclprocessor_frameset, METH_VARARGS, dclprocessor_frameset_docs},

    // Frameset
    {"framesCount", (PyCFunction)dclprocessor_framesCount, METH_VARARGS, dclprocessor_framesCount_docs},
    {"framesIDs", (PyCFunction)dclprocessor_framesIDs, METH_VARARGS, dclprocessor_framesIDs_docs},
    {"frame", (PyCFunction)dclprocessor_frame, METH_VARARGS, dclprocessor_frame_docs},

    // Frameset
    {"image", (PyCFunction)dclprocessor_image, METH_VARARGS, dclprocessor_image_docs},

    // end
    {NULL, NULL, 0, NULL}
};

/*
Имя модуля и это блок его инициализации.
<желаемое имя модуля>, <таблица информации>, <документация модуля>
*/

static struct PyModuleDef dclprocessor_module = {
    PyModuleDef_HEAD_INIT,
    "dclprocessor",
    "This module added all elements of list",
    -1,
    dclprocessor_funcs
};

// Этот метод вызывается при импорте кода в Python. Он создает экземпляр модуля и возвращает ссылку на него
PyMODINIT_FUNC PyInit_dclprocessor(void) {
    import_array();
    return PyModule_Create(&dclprocessor_module); 
}




