#define PY_SSIZE_T_CLEAN
#include "sequence.h"

Py_ssize_t CircularBuffer_length(CircularBuffer* self)
{
    return circularbuffer_total_length(self);
}


PyObject* CircularBuffer_get_item(CircularBuffer* self, Py_ssize_t pos)
{
    Py_ssize_t translated_pos = circularbuffer_translated_position(self, pos);
    if (translated_pos < 0)
    {
        PyErr_SetNone(PyExc_IndexError);
        return NULL;
    }
    return Py_BuildValue(STR_FORMAT_BYTE, &self->raw[translated_pos], 1);
}


int CircularBuffer_set_item(CircularBuffer* self, Py_ssize_t pos,
        PyObject* item)
{
    const char* new_item = PyBytes_AsString(item);
    Py_ssize_t translated_pos = circularbuffer_translated_position(self, pos);
    if (translated_pos < 0)
    {
        PyErr_SetNone(PyExc_IndexError);
        return (int)translated_pos;
    }
    self->raw[translated_pos] = new_item[0];
    return 0;
}


int CircularBuffer_contains(CircularBuffer* self, PyObject* item)
{
    const char* search = PyBytes_AsString(item);
    Py_ssize_t search_len = strlen(search);
    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        return 0;
    }
    const char* psearch = search;
    const char* psearch_end = psearch + search_len;

    const char* pread = circularbuffer_readptr(self);
    const char* pread_half_end;
    const char* pread_end;
    if (self->write < self->read)
    {
        pread_half_end = self->raw + self->allocated_before_resize;
        pread_end = self->raw + self->write;
    }
    else
    {
        pread_half_end = self->raw + self->write;
        pread_end = self->raw + self->write;
    }

    while (pread < pread_half_end)
    {
        psearch = pread[0] == psearch[0] ? psearch + 1 : search;
        if (psearch == psearch_end)
        {
            return 1;
        }
        pread += 1;
    }
    while (pread < pread_end)
    {
        psearch = pread[0] == psearch[0] ? psearch + 1 : search;
        if (psearch == psearch_end)
        {
            return 1;
        }
        pread += 1;
    }

    return 0;
}


PyObject* CircularBuffer_get_slice(CircularBuffer* self,
        Py_ssize_t start, Py_ssize_t end)
{
    return circularbuffer_peek_partial(self, start, end);
}


int CircularBuffer_set_slice(CircularBuffer* self, Py_ssize_t start,
        Py_ssize_t end, PyObject* data)
{
    PyErr_SetNone(PyExc_NotImplementedError);
    return -1;
}


PySequenceMethods CircularBuffer_sequence[] = {
    (lenfunc) CircularBuffer_length,                 // sq_length
    0,                                               // sq_concat
    0,                                               // sq_repeat
    (ssizeargfunc) CircularBuffer_get_item,          // sq_item
    (ssizessizeargfunc) CircularBuffer_get_slice,    // sq_slice
    (ssizeobjargproc) CircularBuffer_set_item,       // sq_ass_item
    0,//(ssizessizeobjargproc) CircularBuffer_set_slice, // sq_ass_slice
    (objobjproc) CircularBuffer_contains,            // sq_contains
    0,                                               // sq_inplace_concat
    0,                                               // sq_inplace_repeat
};
