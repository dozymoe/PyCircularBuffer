#include "buffer.h"

int CircularBuffer_py3_get_buffer(CircularBuffer* self,
        Py_buffer* view, int flags)
{
    printf("CircularBuffer_py3_get_buffer, flags: %i\n", flags);
    //PyErr_SetNone(PyExc_BufferError);
    //view->obj = NULL;
    //return -1;

    Py_ssize_t len = circularbuffer_total_length(self);

    view->obj = (PyObject*) self;
    view->len = len * sizeof(char);
    view->format = "b";

    if (self->write >= self->read)
    {
        view->buf = &self->raw[self->read];
    }
    else
    {
        //view->ndim = 0;
        //self->buf_shape[0] = len;
        //view->itemsize = sizeof(char);
        //view->shape = (void*) &self->buf_shape[0];
        //view->strides = &view->itemsize;
        //view->internal = NULL;
    }
    Py_INCREF(self);
    self->buf_view_count++;
    return 0;
}


int CircularBuffer_py3_release_buffer(CircularBuffer* self,
        Py_buffer* view)
{
    printf("CircularBuffer_py3_release_buffer\n");
    self->buf_view_count--;
    Py_DECREF(self);
    return 0;
}


int CircularBuffer_py2_get_read_buffer(CircularBuffer* self, int segment,
        void** data)
{
    int start = segment == 1 ? 0 : self->read;
    *data = (void*) &self->raw[start];
    return circularbuffer_forward_length(self, start);
}


int CircularBuffer_py2_get_write_buffer(CircularBuffer* self,
        int segment, void** data)
{
    return CircularBuffer_py2_get_read_buffer(self, segment, data);
}


int CircularBuffer_py2_get_segcount(CircularBuffer* self, int* len)
{
    if (len)
    {
        *len = circularbuffer_total_length(self);
    }
    return self->write >= self->read ? 1 : 2;
}


int CircularBuffer_py2_get_char_buffer(CircularBuffer* self, int segment,
        char** data)
{
    int start = segment == 1 ? 0 : self->read;
    *data = (char*) &self->raw[start];
    return circularbuffer_forward_length(self, start);
}


#if PY_MAJOR_VERSION < 3
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
