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
    #define STR_FORMAT_BYTE "s#"
#else
    #define STR_FORMAT_BYTE "y#"
#endif

#define REPR_LENGTH 64

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

    // read-only lock (rare, when restructuring internal buffer)
    int read_lock;
    // read and update-read-pointer lock (when buffer protocol is active)
    char read_write_lock;
#if PY_MAJOR_VERSION < 3
    char buffer_view_count;
#endif
    // write lock (rare, when restructuring internal buffer)
    char write_lock;
} CircularBuffer;


/* custom errors */

extern PyObject* RealignmentError;
extern PyObject* ReservedError;

/* helper functions */

const char* circularbuffer_readptr(CircularBuffer* self);

Py_ssize_t circularbuffer_forward_length(CircularBuffer* self,
        Py_ssize_t start);

Py_ssize_t circularbuffer_total_length(CircularBuffer* self);

Py_ssize_t circularbuffer_forward_available(CircularBuffer* self);
Py_ssize_t circularbuffer_total_available(CircularBuffer* self);

Py_ssize_t circularbuffer_translated_position(CircularBuffer* self,
        Py_ssize_t pos);

Py_ssize_t circularbuffer_write_available(CircularBuffer* self);

PyObject* circularbuffer_peek_partial(CircularBuffer* self,
        Py_ssize_t start, Py_ssize_t size);

int circularbuffer_make_contiguous(CircularBuffer* self);

void circularbuffer_parse_slice_notation(CircularBuffer *self, Py_ssize_t len,
        Py_ssize_t *start, Py_ssize_t *end);

Py_ssize_t circularbuffer_find(CircularBuffer *self, const char* search,
        Py_ssize_t search_len, Py_ssize_t start, Py_ssize_t end);

#endif
