#ifndef CIRCULAR_BUFFER_BUFFER_H
#define CIRCULAR_BUFFER_BUFFER_H

#include "base.h"

int CircularBuffer_py3_get_buffer(CircularBuffer* self, Py_buffer* view,
        int flags);

int CircularBuffer_py3_release_buffer(CircularBuffer* self,
        Py_buffer* view);


int CircularBuffer_py2_get_read_buffer(CircularBuffer* self, int segment,
        void** data);

int CircularBuffer_py2_get_write_buffer(CircularBuffer* self,
        int segment, void** data);

int CircularBuffer_py2_get_segcount(CircularBuffer* self, int* len);

int CircularBuffer_py2_get_char_buffer(CircularBuffer* self, int segment,
        char** data);


extern PyBufferProcs CircularBuffer_buffer[];

#endif
