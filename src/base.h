#ifndef CIRCULAR_BUFFER_BASE_H
#define CIRCULAR_BUFFER_BASE_H

#include <Python.h>
#include <structmember.h>
#include <pyerrors.h>

#ifndef Py_TYPE
    #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif

#define PyString_FromFormat PyUnicode_FromFormat

#if PY_MAJOR_VERSION < 3
    #define STR_FORMAT_BYTE "s"
#else
    #define STR_FORMAT_BYTE "y"
#endif

// see: http://stackoverflow.com/a/17996915
#define QUOTE(...) #__VA_ARGS__

/* objects */

typedef struct {
    PyObject_HEAD
    // type specific fields
    char* raw;
    Py_ssize_t read;
    Py_ssize_t write;
    Py_ssize_t allocated;
    Py_ssize_t allocated_before_resize;
    // buffer protocol
    int buf_view_count;

    char** buf_arr;
    Py_ssize_t buf_shape[2];
    Py_ssize_t buf_strides[2];
    Py_ssize_t buf_suboffsets[2];
} CircularBuffer;


/* helper functions */

const char* circularbuffer_readptr(CircularBuffer* self);

Py_ssize_t circularbuffer_forward_length(CircularBuffer* self,
        Py_ssize_t start);

Py_ssize_t circularbuffer_total_length(CircularBuffer* self);

Py_ssize_t circularbuffer_translated_position(CircularBuffer* self,
        Py_ssize_t pos);

Py_ssize_t circularbuffer_write_available(CircularBuffer* self);

PyObject* circularbuffer_peek_partial(CircularBuffer* self,
        Py_ssize_t start, Py_ssize_t size);


#endif
