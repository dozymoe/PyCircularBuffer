#ifndef CIRCULAR_BUFFER_METHODS_H
#define CIRCULAR_BUFFER_METHODS_H

#include "base.h"

PyObject *CircularBuffer_resize(CircularBuffer *self, PyObject *args,
        PyObject *kwargs);

PyObject *CircularBuffer_read(CircularBuffer *self, PyObject *args,
        PyObject *kwargs);

PyObject* CircularBuffer_write(CircularBuffer* self, PyObject* args,
        PyObject* kwargs);

PyObject* CircularBuffer_write_available(CircularBuffer* self);

PyObject* CircularBuffer_count(CircularBuffer* self, PyObject* args,
        PyObject* kwargs);

PyObject* CircularBuffer_clear(CircularBuffer* self);

PyObject* CircularBuffer_startswith(CircularBuffer* self, PyObject* args,
        PyObject* kwargs);

PyObject* CircularBuffer_find(CircularBuffer* self, PyObject* args,
        PyObject* kwargs);

PyObject* CircularBuffer_index(CircularBuffer* self, PyObject* args,
        PyObject* kwargs);


extern PyMethodDef CircularBuffer_methods[];

#endif
