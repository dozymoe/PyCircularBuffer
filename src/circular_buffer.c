#include <Python.h>
#include <structmember.h>
#include <pyerrors.h>

#ifndef Py_TYPE
    #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif
#ifndef PyString_FromFormat
    #define PyString_FromFormat PyUnicode_FromFormat
#endif
#ifndef PyString_AsString
    #define PyString_AsString PyBytes_AsString
#endif

// see: http://stackoverflow.com/a/17996915
#define QUOTE(...) #__VA_ARGS__

/* objects */

typedef struct {
    PyObject_HEAD
    // type specific fields
    char* raw;
    int read;
    int write;
    int allocated;
    int allocated_before_resize;
} CircularBuffer;


/* helper functions */

/*
 * Get read pointer.
 */
const char* circularbuffer_readptr(CircularBuffer* self)
{
    return (const char*) &self->raw[self->read];
}

/*
 * Size of sequential stored data.
 */
int circularbuffer_forward_length(CircularBuffer* self, int start)
{
    if (self->write >= start)
    {
        return self->write - start;
    }
    else
    {
        return self->allocated_before_resize - start;
    }
}

/*
 * Total size of stored data.
 */
int circularbuffer_total_length(CircularBuffer* self)
{
    int len = circularbuffer_forward_length(self, self->read);
    if (self->write < self->read)
    {
        len += circularbuffer_forward_length(self, 0);
    }
    return len;
}

/*
 * Actual position in our circular bufer.
 */
int circularbuffer_translated_position(CircularBuffer* self, int pos)
{
    if (pos < 0)
    {
        return pos;
    }
    else if (pos > circularbuffer_total_length(self))
    {
        return -1;
    }
    int translated_pos = self->read + pos;
    if (translated_pos > self->allocated_before_resize)
    {
        translated_pos -= self->allocated_before_resize;
    }
    return translated_pos;
}

/*
 * Get sequential size available.
 */
int circularbuffer_write_available(CircularBuffer* self)
{
    if (self->write < self->read)
    {
        return self->read - self->write - 1;
    }
    else if (self->write == self->allocated)
    {
        return self->read - 1;
    }
    else
    {
        return self->allocated - self->write;
    }
}


/* magic methods */

static PyObject* CircularBuffer_create(PyTypeObject* type, PyObject* args,
        PyObject* kwargs)
{
    CircularBuffer* self;

    self = (CircularBuffer*) type->tp_alloc(type, 0);
    if (self)
    {
        self->raw = NULL;
        self->read = 0;
        self->write = 0;
        self->allocated = 0;
        self->allocated_before_resize = 0;
    }

    return (PyObject*) self;
}

static int CircularBuffer_initialize(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"size", NULL};

    int size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "I", kwlist, &size))
    {
        return -1;
    }

    self->raw = (char*) malloc(size + 1);
    if (self->raw == NULL) { return -1; }

    self->allocated = size;
    self->allocated_before_resize = size;
    self->raw[0] = 0;
    return 0;
}

static void CircularBuffer_destroy(CircularBuffer* self)
{
    free(self->raw);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyObject* CircularBuffer_repr(CircularBuffer* self)
{
    if (self->write >= self->read)
    {
        return PyString_FromFormat("<CircularBuffer[%u]:%s>",
                circularbuffer_total_length(self),
                circularbuffer_readptr(self));
    }
    else
    {
        return PyString_FromFormat("<CircularBuffer[%u]:%s%s>",
                circularbuffer_total_length(self),
                circularbuffer_readptr(self),
                self->raw);
    }
}

static PyObject* CircularBuffer_str(CircularBuffer* self)
{
    if (self->write >= self->read)
    {
        return PyString_FromFormat("%s", circularbuffer_readptr(self));
    }
    else
    {
        return PyString_FromFormat("%s%s", circularbuffer_readptr(self),
                self->raw);
    }
}


/* sequence methods */

static Py_ssize_t CircularBuffer_length(CircularBuffer* self)
{
    return circularbuffer_total_length(self);
}

static PyObject* CircularBuffer_get_item(CircularBuffer* self, Py_ssize_t pos)
{
    int translated_pos = circularbuffer_translated_position(self, pos);
    if (translated_pos < 0)
    {
        PyErr_SetNone(PyExc_IndexError);
        return NULL;
    }
#if PY_MAJOR_VERSION >= 3
    return Py_BuildValue("y#", &self->raw[translated_pos], 1);
#else
    return Py_BuildValue("s#", &self->raw[translated_pos], 1);
#endif
}

static int CircularBuffer_set_item(CircularBuffer* self, Py_ssize_t pos,
        PyObject* item)
{
    const char* new_item = PyString_AsString(item);
    int translated_pos = circularbuffer_translated_position(self, pos);
    if (translated_pos < 0)
    {
        PyErr_SetNone(PyExc_IndexError);
        return translated_pos;
    }
    self->raw[translated_pos] = new_item[0];
    return 0;
}

static int CircularBuffer_contains(CircularBuffer* self, PyObject* item)
{
    const char* search = PyString_AsString(item);
    int search_len = strlen(search);
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


/* buffer methods */

static int CircularBuffer_py3_getbuffer(PyObject* exporter, Py_buffer* view,
        int flags)
{
}

static int CircularBuffer_py3_releasebuffer


/* regular methods */

static const char CIRCULARBUFFER_RESIZE_DOCSTRING[] = QUOTE(
    Increase the size of internal buffer.\n
    \n
    :param size: new buffer size\n
    :returns: actual size of the new buffer\n
    :raises MemoryError: cannot allocate memory needed
);

static PyObject *CircularBuffer_resize(CircularBuffer *self, PyObject *args,
        PyObject *kwargs)
{
    static char *kwlist[] = {"size", NULL};
    int size;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "I", kwlist, &size))
    {
        return NULL;
    }
    else if (size > self->allocated)
    {
        void *new_raw = realloc(self->raw, size + 1);
        if (new_raw)
        {
            self->raw = (char*) new_raw;
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

    return Py_BuildValue("I", self->allocated);
}


static const char CIRCULARBUFFER_READ_DOCSTRING[] = QUOTE(
    Read from internal buffer.\n
    \n
    :param size: number of bytes to read, could be negative which means
                 to read all from internal buffer\n
    :returns: bytearray of data whose size could be smaller than requested
);

static PyObject *CircularBuffer_read(CircularBuffer *self, PyObject *args,
        PyObject *kwargs)
{
    static char *kwlist[] = {"size", NULL};
    int size;
    const char* pread = circularbuffer_readptr(self);

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &size))
    {
        return NULL;
    }
    if (size == 0)
    {
        return NULL;
    }
    else if (size < 0 || size == circularbuffer_total_length(self))
    {
#if PY_MAJOR_VERSION >= 3
        PyObject* result = Py_BuildValue("y", pread);
#else
        PyObject* result = Py_BuildValue("s", pread);
#endif
        self->read = self->write;
        return result;
    }
    else
    {
        char* out = (char*) malloc(size + 1);
        out[size] = 0;
        char* pout = out;

        int halflen = circularbuffer_forward_length(self, self->read);

        if (size < halflen || self->write > self->read)
        {
            memcpy(pout, pread, size);
            self->read += size;
        }
        else
        {
            memcpy(pout, pread, halflen);
            pout += halflen;
            size -= halflen;
            self->read = 0;

            pread = circularbuffer_readptr(self);
            memcpy(pout, pread, size);
            self->read += size;
        }

#if PY_MAJOR_VERSION >= 3
        PyObject* result = Py_BuildValue("y", out);
#else
        PyObject* result = Py_BuildValue("s", out);
#endif
        free(out);
        return result;
    }
}


static const char CIRCULARBUFFER_WRITE_DOCSTRING[] = QUOTE(
    Write into internal buffer.\n
    Type of bytes is expected, and unicode will be automatically encoded with
    utf-8.\n
    \n
    :param data: bytearray to be added to the buffer\n
    :returns: number of bytes written, could be less than the size of data
);

static PyObject* CircularBuffer_write(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"data", NULL};

    const char* data;

#if PY_MAJOR_VERSION >= 3
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y", kwlist, &data))
#else
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &data))
#endif
    {
        return NULL;
    }

    int avail = circularbuffer_write_available(self);
    int length = strlen(data);

    if (avail == 0) {
        return Py_BuildValue("I", 0);
    }
    else if (length > avail)
    {
        length = avail;
    }
    if (self->write == self->allocated)
    {
        self->write = 0;
    }
    memcpy(&self->raw[self->write], data, length);
    self->write += length;
    self->raw[self->write] = 0;

    if (self->allocated_before_resize < self->allocated && \
            self->write >= self->read)
    {
        self->allocated_before_resize = self->allocated;
    }

    return Py_BuildValue("I", length);
}


static const char CIRCULARBUFFER_WRITE_AVAILABLE_DOCSTRING[] = QUOTE(
    Size of internal buffer available for writing.\n
    \n
    :returns: size of one half of internal buffer available
);

static PyObject* CircularBuffer_write_available(CircularBuffer* self)
{
    int size = circularbuffer_write_available(self);
    return Py_BuildValue("I", size);
}


static const char CIRCULARBUFFER_COUNT_DOCSTRING[] = QUOTE(
    Return the number of occurences of string in internal buffer.\n
    \n
    :param text: string to search\n
    :returns: number of occurences
);

static PyObject* CircularBuffer_count(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"text", NULL};

    const char* search;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &search))
    {
        PyErr_BadArgument();
        return NULL;
    }

    int search_len = strlen(search);
    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_BadArgument();
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

static PyObject* CircularBuffer_clear(CircularBuffer* self)
{
    self->write = self->read = 0;
    self->raw[0] = '\0';
    self->allocated_before_resize = self->allocated;
    return Py_BuildValue("s", NULL);
}


static const char CIRCULARBUFFER_STARTSWITH_DOCSTRING[] = QUOTE(
    CB.startswith(prefix) -> bool\n
    \n
    Return True if S starts with the specified prefix, False otherwise.
);

static PyObject* CircularBuffer_startswith(CircularBuffer* self, PyObject* args,
        PyObject* kwargs)
{
    static char* kwlist[] = {"prefix", NULL};

    const char* search;

#if PY_MAJOR_VERSION >= 3
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "y", kwlist, &search))
#else
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &search))
#endif
    {
        PyErr_BadArgument();
        return NULL;
    }

    int search_len = strlen(search);
    if (search_len > circularbuffer_total_length(self) || search_len == 0)
    {
        PyErr_BadArgument();
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


/* meta description */

static PyMethodDef Module_methods[] = {
    // end of array
    {NULL},
};

static PyMemberDef CircularBuffer_members[] = {
    // end of array
    {NULL},
};

static PyMethodDef CircularBuffer_methods[] = {
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

static PySequenceMethods CircularBuffer_sequence_methods[] = {
    (lenfunc) CircularBuffer_length,             // sq_length
    0,                                           // sq_concat
    0,                                           // sq_repeat
    (ssizeargfunc) CircularBuffer_get_item,      // sq_item
    0,                                           // sq_slice
    (ssizeobjargproc) CircularBuffer_set_item,   // sq_ass_item
    0,                                           // sq_ass_slice
    (objobjproc) CircularBuffer_contains,        // sq_contains
    0,                                           // sq_inplace_concat
    0,                                           // sq_inplace_repeat
};

#if PY_MAJOR_VERSION < 3
static PyBufferProcs CircularBuffer_buffer_methods[] = {
    readbufferproc bf_getreadbuffer,              // bf_getreadbuffer
    writebufferproc bf_getwritebuffer,            // bf_getwritebuffer
    segcountproc bf_getsegcount,                  // bf_getsegcount
    charbufferproc bf_getcharbuffer,              // bf_getcharbuffer
    (getbufferproc) CircularBuffer_py3_getbuffer, // bf_getbuffer
    0,                                            // bf_releasebuffer
};
#else
static PyBufferProcs CircularBuffer_buffer_methods[] = {
    (getbufferproc) CircularBuffer_py3_getbuffer, // bf_getbuffer
    0,                                            // bf_releasebuffer
};
#endif

// see: https://docs.python.org/2/c-api/typeobj.html

static PyTypeObject CircularBufferType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "circularbuffer.CircularBuffer",           // tp_name
    sizeof(CircularBuffer),                    // tp_basicsize
    0,                                         // tp_itemsize
    (destructor) CircularBuffer_destroy,       // tp_dealloc
    0,                                         // tp_print (deprecated)
    0,                                         // tp_getattr (deprecated)
    0,                                         // tp_setattr (deprecated)
    0,                                         // tp_compare
    (reprfunc) CircularBuffer_repr,            // tp_repr
    0,                                         // tp_as_number
    CircularBuffer_sequence_methods,           // tp_as_sequence
    0,                                         // tp_as_mapping
    0,                                         // tp_hash
    0,                                         // tp_call
    (reprfunc) CircularBuffer_str,             // tp_str
    0,                                         // tp_getattro
    0,                                         // tp_setattro
    CircularBuffer_buffer_methods,             // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  // tp_flags
    "Circular buffer",                         // tp_doc
    0,                                         // tp_traverse
    0,                                         // tp_clear
    0,                                         // tp_richcompare
    0,                                         // tp_weaklistoffset
    0,                                         // tp_iter
    0,                                         // tp_iternext
    CircularBuffer_methods,                    // tp_methods
    CircularBuffer_members,                    // tp_members
    0,                                         // tp_getset
    0,                                         // tp_base
    0,                                         // tp_dict
    0,                                         // tp_descr_get
    0,                                         // tp_descr_set
    0,                                         // tp_dictoffset
    (initproc) CircularBuffer_initialize,      // tp_init
    0,                                         // tp_alloc
    CircularBuffer_create,                     // tp_new
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "circularbuffer",                           /* m_name */
    "Simple implementation of circular bufer.", /* m_doc */
    -1,                                         /* m_size */
    Module_methods,                             /* m_methods */
    NULL,                                       /* m_reload */
    NULL,                                       /* m_traverse */
    NULL,                                       /* m_clear */
    NULL,                                       /* m_free */
};
#endif


/* initialization */

static PyObject *module_init(void)
{
    // create new module
    #if PY_MAJOR_VERSION >= 3
    PyObject* module = PyModule_Create(&moduledef);
    #else
    PyObject* module = Py_InitModule3("circularbuffer", Module_methods,
        "Simple implementation of circular buffer."
    );
    #endif
    if (module == NULL) { return NULL; }

    // create new class
    if (PyType_Ready(&CircularBufferType) < 0) { return NULL; }

    // add class to module
    Py_INCREF(&CircularBufferType);
    PyModule_AddObject(module, "CircularBuffer",
                       (PyObject*) &CircularBufferType);

    return module;
}

#if PY_MAJOR_VERSION < 3
PyMODINIT_FUNC initcircularbuffer(void) {
    module_init();
}
#else
PyMODINIT_FUNC PyInit_circularbuffer(void) {
    return module_init();
}
#endif
