#include "methods.h"

static const char CIRCULARBUFFER_RESIZE_DOCSTRING[] = QUOTE(
    Increase the size of internal buffer.\n
    \n
    :param size: new buffer size\n
    :returns: actual size of the new buffer\n
    :raises MemoryError: cannot allocate memory needed
);

PyObject *CircularBuffer_resize(CircularBuffer *self, PyObject *args,
        PyObject *kwargs)
{
    static char *kwlist[] = {"size", NULL};
    Py_ssize_t size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "n", kwlist, &size))
    {
        return NULL;
    }
    else if (size > self->allocated)
    {
        void *new_raw = PyMem_Realloc(self->raw, size + 2);
        if (new_raw)
        {
            self->raw = (char*) new_raw;
            self->raw[size + 1] = 0;
            self->allocated = size;
            if (self->write >= self->read)
            {
                self->allocated_before_resize = size;
            }
        }
        else
        {
            return PyErr_NoMemory();
        }
    }

    return Py_BuildValue("n", self->allocated);
}


static const char CIRCULARBUFFER_READ_DOCSTRING[] = QUOTE(
    Read from internal buffer.\n
    \n
    :param size: number of bytes to read, could be negative which means
                 to read all from internal buffer\n
    :returns: bytearray of data whose size could be smaller than requested
);

PyObject *CircularBuffer_read(CircularBuffer *self, PyObject *args,
        PyObject *kwargs)
{
    static char *kwlist[] = {"size", NULL};
    Py_ssize_t size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "n", kwlist, &size))
    {
        return NULL;
    }
    else if (self->buf_view_count != 0)
    {
        PyErr_SetString(PyExc_RuntimeError, "Cannot read while buffer "
                "protocol is still active.");

        return NULL;
    }
    else if (size == 0)
    {
        return Py_BuildValue(STR_FORMAT_BYTE, "");
    }

    if (size < 0)
    {
        size = circularbuffer_total_length(self);
    }

    PyObject* result = circularbuffer_peek_partial(self, 0, size);

    self->read += size;
    if (self->read > self->allocated_before_resize)
    {
        self->read -= self->allocated_before_resize + 1;
    }

    return result;
}


static const char CIRCULARBUFFER_WRITE_DOCSTRING[] = QUOTE(
    Write into internal buffer.\n
    Type of bytes is expected, and unicode will be automatically encoded with
    utf-8.\n
    \n
    :param data: bytearray to be added to the buffer\n
    :returns: number of bytes written, could be less than the size of data
);

PyObject* CircularBuffer_write(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"data", NULL};

    const char* data;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE, kwlist,
            &data))
    {
        return NULL;
    }

    Py_ssize_t written = 0;
    Py_ssize_t length = strlen(data);

    // two halves
    while (length)
    {
        Py_ssize_t avail = circularbuffer_forward_available(self);
        if (avail == 0)
        {
            break;
        }
        else if (self->write == self->allocated  + 1)
        {
            self->write = 0;
        }

        Py_ssize_t count = length > avail ? avail : length;
        memcpy(&self->raw[self->write], data, count);

        length -= count;
        data += count;
        written += count;

        self->write += count;
        self->raw[self->write] = 0;
    }

    return Py_BuildValue("n", written);
}


static const char CIRCULARBUFFER_WRITE_AVAILABLE_DOCSTRING[] = QUOTE(
    Size of internal buffer available for writing.\n
    \n
    :returns: size of one half of internal buffer available
);

PyObject* CircularBuffer_write_available(CircularBuffer* self)
{
    Py_ssize_t size = circularbuffer_total_available(self);
    return Py_BuildValue("n", size);
}


static const char CIRCULARBUFFER_COUNT_DOCSTRING[] = QUOTE(
    Return the number of occurences of string in internal buffer.\n
    \n
    :param text: string to search\n
    :returns: number of occurences
);

PyObject* CircularBuffer_count(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"text", NULL};

    const char* search;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE, kwlist,
            &search))
    {
        return NULL;
    }

    int search_len = strlen(search);
    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_SetString(PyExc_ValueError, "invalid search string length");
        return NULL;
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

    Py_ssize_t count = 0;

    while (pread < pread_half_end)
    {
        psearch = pread[0] == psearch[0] ? psearch + 1 : search;
        if (psearch == psearch_end)
        {
            count += 1;
            psearch = search;
        }
        pread += 1;
    }
    while (pread < pread_end)
    {
        psearch = pread[0] == psearch[0] ? psearch + 1 : search;
        if (psearch == psearch_end)
        {
            count += 1;
            psearch = search;
        }
        pread += 1;
    }

    return Py_BuildValue("n", count);
}


static const char CIRCULARBUFFER_CLEAR_DOCSTRING[] = QUOTE(
    Size of internal buffer available for writing.\n
    \n
    :returns: size of one half of internal buffer available
);

PyObject* CircularBuffer_clear(CircularBuffer* self)
{
    self->write = self->read = 0;
    self->raw[0] = 0;
    self->allocated_before_resize = self->allocated;
    Py_RETURN_NONE;
}


static const char CIRCULARBUFFER_STARTSWITH_DOCSTRING[] = QUOTE(
    CB.startswith(prefix) -> bool\n
    \n
    Return True if S starts with the specified prefix, False otherwise.
);

PyObject* CircularBuffer_startswith(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"prefix", NULL};

    const char* search;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE, kwlist,
            &search))
    {
        return NULL;
    }

    int search_len = strlen(search);
    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_SetString(PyExc_ValueError, "invalid search string length");
        return NULL;
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

    Py_ssize_t count = 0;

    while (pread < pread_half_end)
    {
        if (pread[0] == psearch[0])
        {
            psearch += 1;
        }
        else
        {
            return Py_BuildValue("i", 0);
        }
        if (psearch == psearch_end)
        {
            return Py_BuildValue("i", 1);
        }
        pread += 1;
    }
    while (pread < pread_end)
    {
        if (pread[0] == psearch[0])
        {
            psearch += 1;
        }
        else
        {
            return Py_BuildValue("i", 0);
        }
        if (psearch == psearch_end)
        {
            return Py_BuildValue("i", 1);
        }
        pread += 1;
    }

    return Py_BuildValue("i", 0);
}


PyMethodDef CircularBuffer_methods[] = {
    {
        "clear",
        (PyCFunction) CircularBuffer_clear,
        METH_NOARGS,
        CIRCULARBUFFER_CLEAR_DOCSTRING
    },
    {
        "count",
        (PyCFunction) CircularBuffer_count,
        METH_VARARGS | METH_KEYWORDS,
        CIRCULARBUFFER_COUNT_DOCSTRING
    },
    {
        "read",
        (PyCFunction) CircularBuffer_read,
        METH_VARARGS | METH_KEYWORDS,
        CIRCULARBUFFER_READ_DOCSTRING
    },
    {
        "resize",
        (PyCFunction) CircularBuffer_resize,
        METH_VARARGS | METH_KEYWORDS,
        CIRCULARBUFFER_RESIZE_DOCSTRING
    },
    {
        "startswith",
        (PyCFunction) CircularBuffer_startswith,
        METH_VARARGS | METH_KEYWORDS,
        CIRCULARBUFFER_STARTSWITH_DOCSTRING
    },
    {
        "write",
        (PyCFunction) CircularBuffer_write,
        METH_VARARGS | METH_KEYWORDS,
        CIRCULARBUFFER_WRITE_DOCSTRING
    },
    {
        "write_available",
        (PyCFunction) CircularBuffer_write_available,
        METH_NOARGS,
        CIRCULARBUFFER_WRITE_AVAILABLE_DOCSTRING
    },
    // end of array
    {NULL},
};

