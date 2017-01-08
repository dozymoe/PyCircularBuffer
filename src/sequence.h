#ifndef CIRCULAR_BUFFER_SEQUENCE_H
#define CIRCULAR_BUFFER_SEQUENCE_H

#include "base.h"

Py_ssize_t CircularBuffer_length(CircularBuffer* self);

PyObject* CircularBuffer_get_item(CircularBuffer* self, Py_ssize_t pos);

int CircularBuffer_set_item(CircularBuffer* self, Py_ssize_t pos,
        PyObject* item);

int CircularBuffer_contains(CircularBuffer* self, PyObject* item);

PyObject* CircularBuffer_get_slice(CircularBuffer* self,
        Py_ssize_t start, Py_ssize_t end);

int CircularBuffer_set_slice(CircularBuffer* self, Py_ssize_t start,
        Py_ssize_t end, PyObject* data);

extern PySequenceMethods CircularBuffer_sequence[];

#endif
