#ifndef CIRCULAR_BUFFER_MAPPING_H
#define CIRCULAR_BUFFER_MAPPING_H

#include "base.h"

PyObject* CircularBuffer_get_subscript(CircularBuffer* self,
        PyObject* item);

int CircularBuffer_set_subscript(CircularBuffer* self, PyObject* item,
        PyObject* value);


extern PyMappingMethods CircularBuffer_mapping[];

#endif
