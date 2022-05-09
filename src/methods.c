#define PY_SSIZE_T_CLEAN
#include "base.h"
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
    :returns: bytearray of data whose size could be smaller than requested\n
    :raises ReservedError: someone uses buffer protocol
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
    else if (self->read_lock || self->read_write_lock)
    {
        PyErr_SetString(ReservedError, "The internal buffer cannot be modified "
                "at the moment.");

        return NULL;
    }
    else if (size == 0)
    {
        return Py_BuildValue(STR_FORMAT_BYTE, "", 0);
    }

    Py_ssize_t len = circularbuffer_total_length(self);
    Py_ssize_t start = 0;
    Py_ssize_t end = size;

    circularbuffer_parse_slice_notation(self, len, &start, &end);

    if (end <= start || start < 0 || end < 0)
    {
        return Py_BuildValue(STR_FORMAT_BYTE, "", 0);
    }

    PyObject* result = circularbuffer_peek_partial(self, 0, end);

    self->read += end - start;
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
    :returns: number of bytes written, could be less than the size of data\n
    :raises RealignmentError: internal buffer is being realign into one segment
);

PyObject* CircularBuffer_write(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"data", NULL};

    const char* data;
    Py_ssize_t length;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE, kwlist,
            &data, &length))
    {
        return NULL;
    }
    else if (self->write_lock)
    {
        PyErr_SetString(RealignmentError, "This is rare, but internal buffer "
                "temporarily not available.");

        return NULL;
    }

    Py_ssize_t written = 0;

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
    :returns: number of occurences\n
    :raises RealignmentError: internal buffer is being realign into one segment
);

PyObject* CircularBuffer_count(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"text", NULL};

    const char* search;
    Py_ssize_t search_len;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE, kwlist,
            &search, &search_len))
    {
        return NULL;
    }
    else if (self->read_lock)
    {
        PyErr_SetString(RealignmentError, "This is rare, but internal buffer "
                "temporarily not available.");

        return NULL;
    }

    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_SetString(PyExc_ValueError, "invalid search string length");
        return NULL;
    }

    const char* psearch = search;
    const char* psearch_end = psearch + search_len;

    const char* pread = circularbuffer_readptr(self);
    const char* pread_half_end;
    if (self->write < self->read)
    {
        pread_half_end = self->raw + self->allocated_before_resize;
    }
    else
    {
        pread_half_end = self->raw + self->write;
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
    if (self->write < self->read)
    {
        pread = self->raw;
        pread_half_end = self->raw + self->write;

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
    }
    return Py_BuildValue("n", count);
}


static const char CIRCULARBUFFER_CLEAR_DOCSTRING[] = QUOTE(
    Size of internal buffer available for writing.\n
    \n
    :returns: size of one half of internal buffer available\n
    :raises ReservedError: someone uses buffer protocol
);

PyObject* CircularBuffer_clear(CircularBuffer* self)
{
    if (self->write_lock || self->read_write_lock)
    {
        PyErr_SetString(ReservedError, "The internal buffer cannot be modified "
                "at the moment.");

        return NULL;
    }
    self->write = self->read = 0;
    self->raw[0] = 0;
    self->allocated_before_resize = self->allocated;
    Py_RETURN_NONE;
}


static const char CIRCULARBUFFER_STARTSWITH_DOCSTRING[] = QUOTE(
    CB.startswith(prefix) -> bool\n
    \n
    Return True if S starts with the specified prefix, False otherwise.\n
    :raises RealignmentError: internal buffer is being realign into one segment
);

PyObject* CircularBuffer_startswith(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"prefix", NULL};

    const char* search;
    Py_ssize_t search_len;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE, kwlist,
            &search, &search_len))
    {
        return NULL;
    }
    else if (self->read_lock)
    {
        PyErr_SetString(RealignmentError, "This is rare, but internal buffer "
                "temporarily not available.");

        return NULL;
    }

    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_SetString(PyExc_ValueError, "Invalid search string length.");
        return NULL;
    }

    Py_ssize_t pos = circularbuffer_find(self, search, search_len, 0,
            search_len);

    if (pos < 0)
    {
        return Py_BuildValue("i", 0);
    }
    else
    {
        return Py_BuildValue("i", 1);
    }
}


static const char CIRCULARBUFFER_FIND_DOCSTRING[] = QUOTE(
    CB.find(sub [,start [,end]]) -> int\n
    \n
    Return the lowest index in CB where substring sub is found,\n
    such that sub is contained within CB[start:end]. Optional\n
    arguments start and end are interpreted as in slice notation.\n
    \n
    Return -1 on failure.\n
    \n
    :param sub: string to search\n
    :param start: index for partial search\n
    :param end: index for partial search\n
    :returns: index of first occurence\n
);

PyObject* CircularBuffer_find(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"sub", "start", "end", NULL};

    const char* search;
    Py_ssize_t search_len;
    Py_ssize_t start = 0;
    Py_ssize_t end = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE "|nn",
            kwlist, &search, &search_len, &start, &end))
    {
        return NULL;
    }
    else if (self->read_lock)
    {
        PyErr_SetString(RealignmentError, "This is rare, but internal buffer "
                "temporarily not available.");

        return NULL;
    }

    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_SetString(PyExc_ValueError, "invalid search string length");
        return NULL;
    }

    return Py_BuildValue("n", circularbuffer_find(self, search, search_len,
                start, end));
}


static const char CIRCULARBUFFER_INDEX_DOCSTRING[] = QUOTE(
    CB.index(sub [,start [,end]]) -> int\n
    \n
    Like CB.find() but raise ValueError when the substring is not found.\n
    \n
    :param sub: string to search\n
    :param start: index for partial search\n
    :param end: index for partial search\n
    :returns: number of occurences\n
    :raises ValueError: unable to find sub\n
);

PyObject* CircularBuffer_index(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"sub", "start", "end", NULL};

    const char* search;
    Py_ssize_t search_len;
    Py_ssize_t start = 0;
    Py_ssize_t end = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, STR_FORMAT_BYTE "|nn",
            kwlist, &search, &search_len, &start, &end))
    {
        return NULL;
    }
    else if (self->read_lock)
    {
        PyErr_SetString(RealignmentError, "This is rare, but internal buffer "
                "temporarily not available.");

        return NULL;
    }

    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_SetString(PyExc_ValueError, "invalid search string length");
        return NULL;
    }

    Py_ssize_t pos = circularbuffer_find(self, search, search_len, start, end);
    if (pos < 0)
    {
        PyErr_SetString(PyExc_ValueError, "substring not found");
        return NULL;
    }
    else
    {
        return Py_BuildValue("n", pos);
    }
}


static const char CIRCULARBUFFER_MAKE_CONTIGUOUS_DOCSTRING[] = QUOTE(
    CB.make_contiguous() -> None\n
    \n
    Reallign internal buffer into one segment, used by buffer protocol.\n
    :raises ReservedError: someone uses buffer protocol
);

PyObject* CircularBuffer_make_contiguous(CircularBuffer* self)
{
    if (circularbuffer_make_contiguous(self))
    {
        return NULL;
    }
    Py_RETURN_NONE;
}


static const char CIRCULARBUFFER_CONTEXT_ENTER_DOCSTRING[] = QUOTE(
    CB.__enter__() -> CB\n
    \n
    Lock buffer from modification, you could still write into though.\n
    :raises ReservedError: someone uses buffer protocol
);

PyObject* CircularBuffer_context_enter(CircularBuffer* self)
{
    if (circularbuffer_make_contiguous(self))
    {
        return NULL;
    }
    self->read_write_lock++;
    Py_INCREF(self);
    return (PyObject*)self;
}


static const char CIRCULARBUFFER_CONTEXT_EXIT_DOCSTRING[] = QUOTE(
    CB.__exit__() -> None\n
    \n
    Release lock, allow modification of internal buffer.\n
    :raises ReservedError: someone uses buffer protocol
);

PyObject* CircularBuffer_context_exit(CircularBuffer* self, PyObject* args)
{
    //Py_DECREF(self);
    self->read_write_lock--;
#if PY_MAJOR_VERSION < 3
    self->buffer_view_count--;
#endif
    Py_RETURN_NONE;
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
    {
        "find",
        (PyCFunction) CircularBuffer_find,
        METH_VARARGS | METH_KEYWORDS,
        CIRCULARBUFFER_FIND_DOCSTRING
    },
    {
        "index",
        (PyCFunction) CircularBuffer_index,
        METH_VARARGS | METH_KEYWORDS,
        CIRCULARBUFFER_INDEX_DOCSTRING
    },
    {
        "make_contiguous",
        (PyCFunction) CircularBuffer_make_contiguous,
        METH_NOARGS,
        CIRCULARBUFFER_MAKE_CONTIGUOUS_DOCSTRING
    },
    {
        "__enter__",
        (PyCFunction) CircularBuffer_context_enter,
        METH_NOARGS,
        CIRCULARBUFFER_CONTEXT_ENTER_DOCSTRING
    },
    {
        "__exit__",
        (PyCFunction) CircularBuffer_context_exit,
        METH_VARARGS,
        CIRCULARBUFFER_CONTEXT_EXIT_DOCSTRING
    },
    // end of array
    {NULL},
};

