#define PY_SSIZE_T_CLEAN
#include "buffer.h"

int CircularBuffer_py3_get_buffer(CircularBuffer* self,
        Py_buffer* view, int flags)
{
    if (circularbuffer_make_contiguous(self))
    {
        view->obj = NULL;
        return -1;
    }

    self->read_write_lock++;

    Py_ssize_t len = circularbuffer_total_length(self);

    view->obj = (PyObject*) self;
    view->len = len * sizeof(char);
    view->format = "b";
    view->buf = &self->raw[self->read];

    Py_INCREF(self);
    return 0;
}


int CircularBuffer_py3_release_buffer(CircularBuffer* self,
        Py_buffer* view)
{
    //Py_DECREF(self);
    self->read_write_lock--;
    return 0;
}


#if PY_MAJOR_VERSION < 3

int CircularBuffer_py2_get_read_buffer(CircularBuffer* self, int segment,
        void** data)
{
    *data = (void*) &self->raw[self->read];
    return circularbuffer_total_length(self);
}


int CircularBuffer_py2_get_write_buffer(CircularBuffer* self,
        int segment, void** data)
{
    return CircularBuffer_py2_get_read_buffer(self, segment, data);
}


int CircularBuffer_py2_get_segcount(CircularBuffer* self, int* len)
{
    if (self->read_write_lock <= self->buffer_view_count)
    {
        // please use context manager, the `with` statement
        return -1;
    }

    self->buffer_view_count++;

    if (len)
    {
        *len = circularbuffer_total_length(self);
    }

    // regex doesn't support multiple segments
    return 1;
}


int CircularBuffer_py2_get_char_buffer(CircularBuffer* self, int segment,
        char** data)
{
    *data = &self->raw[self->read];
    return circularbuffer_total_length(self);
}


PyBufferProcs CircularBuffer_buffer[] = {
    (readbufferproc) CircularBuffer_py2_get_read_buffer,   // bf_getreadbuffer
    (writebufferproc) CircularBuffer_py2_get_write_buffer, // bf_getwritebuffer
    (segcountproc) CircularBuffer_py2_get_segcount,        // bf_getsegcount
    (charbufferproc) CircularBuffer_py2_get_char_buffer,   // bf_getcharbuffer
    (getbufferproc) CircularBuffer_py3_get_buffer,         // bf_getbuffer
    (releasebufferproc) CircularBuffer_py3_release_buffer, // bf_releasebuffer
};

#else

PyBufferProcs CircularBuffer_buffer[] = {
    (getbufferproc) CircularBuffer_py3_get_buffer,         // bf_getbuffer
    (releasebufferproc) CircularBuffer_py3_release_buffer, // bf_releasebuffer
};

#endif
