#include <Python.h>
#include <structmember.h>
#include <pyerrors.h>

#ifndef Py_TYPE
    #define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif
#ifndef PyString_FromFormat
    #define PyString_FromFormat PyUnicode_FromFormat
#endif
#ifndef PyString_FromString
    #define PyString_FromString PyUnicode_FromString
#endif

/* objects */

typedef struct {
    PyObject_HEAD
    // type specific fields
    char* raw;
    int read;
    int write;
    int allocated;
} CircularBuffer;


/* helper functions */

const char* circularbuffer_peek(CircularBuffer* buf)
{
    return (const char*) &buf->raw[buf->read];
}

int circularbuffer_length(CircularBuffer* buf)
{
  return strlen(circularbuffer_peek(buf));
}

int circularbuffer_available(CircularBuffer* buf)
{
  if (buf->write < buf->read)
  {
    return buf->read - buf->write - 1;
  }
  else if (buf->write == buf->allocated - 1)
  {
    return buf->read;
  }
  else
  {
    return buf->allocated - buf->write - 1;
  }
}


/* magic methods */

static PyObject *CircularBuffer_create(PyTypeObject *type, PyObject *args,
                                       PyObject *kwargs)
{
    printf("CircularBuffer.__new__() called\n");

    CircularBuffer *self;

    self = (CircularBuffer*) type->tp_alloc(type, 0);
    if (self)
    {
        self->raw = NULL;
        self->read = 0;
        self->write = 0;
        self->allocated = 0;
    }

    return (PyObject*) self;
}

static int CircularBuffer_initialize(CircularBuffer *self, PyObject *args,
                                     PyObject *kwargs)
{
    printf("CircularBuffer.__init__() called\n");

    static char *kwlist[] = {"size", NULL};

    int size;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "I", kwlist, &size))
    {
        return -1;
    }

    self->raw = (char*) malloc(size);
    if (self->raw == NULL) { return -1; }

    self->allocated = size;
    self->raw[0] = 0;
    return 0;
}

static void CircularBuffer_destroy(CircularBuffer *self)
{
    printf("CircularBuffer.__del__() called\n");

    free(self->raw);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyObject *CircularBuffer_repr(CircularBuffer *self)
{
    printf("CircularBuffer.__repr__() called\n");

    return PyString_FromFormat("<CircularBuffer[%i]:%s>",
                               circularbuffer_length(self),
                               circularbuffer_peek(self));
}

static PyObject *CircularBuffer_str(CircularBuffer *self)
{
    printf("CircularBuffer.__str__() called\n");

    return PyString_FromString(circularbuffer_peek(self));
}


/* sequence methods */

static Py_ssize_t CircularBuffer_length(CircularBuffer *self)
{
    printf("CircularBuffer.__len__() called\n");

    return circularbuffer_length(self);
}

static PyObject *CircularBuffer_item(CircularBuffer *self, int pos)
{
    printf("CircularBuffer.__getitem__() called\n");

    if (pos > circularbuffer_length(self)) { return NULL; }
    return Py_BuildValue("s", self->raw[self->read + pos]);
}

static int CircularBuffer_ass_item(CircularBuffer *self, int pos,
                                   PyObject *args)
{
    printf("CircularBuffer.__setitem__() called\n");

    const char *data;


    if (!PyArg_ParseTuple(args, "s", &data)) { return -1; }
    if (pos > circularbuffer_length(self)) { return -1; }
    self->raw[self->read + pos] = data[0];
    return 1;
}

static int CircularBuffer_contains(CircularBuffer *self, PyObject *args)
{
    printf("CircularBuffer.__contains__() called\n");

    const char *search;


    if (!PyArg_Parse(args, "s", &search))
    {
        return -1;
    }

    const char *peek = circularbuffer_peek(self);
    char *pos = strpbrk(peek, search);
    if (pos) { return 1; }
    else { return 0; }
}


/* regular methods */

static PyObject *CircularBuffer_resize(CircularBuffer *self, PyObject *args,
                                       PyObject *kwargs)
{
    printf("CircularBuffer.resize() called\n");

    static char *kwlist[] = {"size", NULL};

    int size;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "I", kwlist, &size))
    {
        return NULL;
    }
    else if (size > self->allocated)
    {
        void *new_raw = realloc(self->raw, size);
        if (new_raw)
        {
            self->raw = (char*) new_raw;
            self->allocated = size;
        }
        else
        {
            // raise exceptions
            PyErr_SetString(PyExc_MemoryError, "Cannot allocate memory");
            return NULL;
        }
    }

    return Py_BuildValue("I", self->allocated);
}

static PyObject *CircularBuffer_read(CircularBuffer *self, PyObject *args,
                                     PyObject *kwargs)
{
    printf("CircularBuffer.read() called\n");

    static char *kwlist[] = {"out", "size", NULL};

    char* out;
    int size;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "w#", kwlist, &out, &size))
    {
        return NULL;
    }

    int length = circularbuffer_length(self);
    if (length == 0 || size <= 1) { return Py_BuildValue("I", 0); }

    // exclude the terminating null character
    size = size - 1;
    // if actual length is smaller than requested
    if (size > length) { size = length; }
    // copy and set final null character
    memcpy(out, circularbuffer_peek(self), size);
    out[size] = 0;
    // move read index
    if (size < length)
    {
        self->read += size;
    }
    else if (self->write < self->read)
    {
        self->read = 0;
    }
    else
    {
        self->read = self->write;
    }
    return Py_BuildValue("I", size);
}

static PyObject *CircularBuffer_write(CircularBuffer *self, PyObject *args,
                                      PyObject *kwargs)
{
    printf("CircularBuffer.write() called\n");

    static char *kwlist[] = {"data", NULL};

    const char* data;


    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &data))
    {
        return NULL;
    }

    int avail = circularbuffer_available(self);
    int length = strlen(data);

    if (avail == 0) {
        return Py_BuildValue("I", 0);
    }
    else if (length > avail)
    {
        length = avail;
    }
    if (self->write == self->allocated - 1)
    {
        self->write = 0;
    }
    memcpy(&self->raw[self->write], data, length);
    self->write += length;
    self->raw[self->write] = 0;

    return Py_BuildValue("I", length);
}

static PyObject *CircularBuffer_available(CircularBuffer *self)
{
    printf("CircularBuffer.available() called\n");

    int size;
    if (self->write < self->read)
    {
        size = self->read - self->write - 1;
    }
    else if (self->write == self->allocated - 1)
    {
        size = self->read;
    }
    else
    {
        size = self->allocated - self->write - 1;
    }
    return Py_BuildValue("I", size);
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
    {"resize", (PyCFunction) CircularBuffer_resize,
     METH_VARARGS | METH_KEYWORDS,
     "Increase the size of the buffer."},
    {"read", (PyCFunction) CircularBuffer_read,
     METH_VARARGS | METH_KEYWORDS,
     "Read from internal buffer."},
    {"write", (PyCFunction) CircularBuffer_write,
     METH_VARARGS | METH_KEYWORDS,
     "Write into internal buffer."},
    {"available", (PyCFunction) CircularBuffer_available,
     METH_NOARGS,
     "Buffer size currently available for writing."},
    // end of array
    {NULL},
};

static PySequenceMethods CircularBuffer_sequence_methods[] = {
    // sq_length
    (lenfunc) CircularBuffer_length,
    0, // sq_concat
    0, // sq_repeat
    // sq_item
    (ssizeargfunc) CircularBuffer_item,
    0, // sq_slice
    // sq_ass_item
    (ssizeobjargproc) CircularBuffer_ass_item,
    0, // sq_ass_slice
    // sq_contains
    (objobjproc) CircularBuffer_contains,
    0, // sq_inplace_concat
    0, // sq_inplace_repeat
};

// see: https://docs.python.org/2/c-api/typeobj.html

static PyTypeObject CircularBufferType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    // tp_name
    "circularbuffer.CircularBuffer",
    // tp_basicsize
    sizeof(CircularBuffer),
    0,  // tp_itemsize
    // tp_dealloc
    (destructor) CircularBuffer_destroy,
    0,  // tp_print (deprecated)
    0,  // tp_getattr (deprecated)
    0,  // tp_setattr (deprecated)
    0,  // tp_compare
    // tp_repr
    (reprfunc) CircularBuffer_repr,
    0,  // tp_as_number
    // tp_as_sequence
    CircularBuffer_sequence_methods,
    0,  // tp_as_mapping
    0,  // tp_hash
    0,  // tp_call
    // tp_str
    (reprfunc) CircularBuffer_str,
    0,  // tp_getattro
    0,  // tp_setattro
    0,  // tp_as_buffer
    // tp_flags
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    // tp_doc
    "Circular buffer",
    0,  // tp_traverse
    0,  // tp_clear
    0,  // tp_richcompare
    0,  // tp_weaklistoffset
    0,  // tp_iter
    0,  // tp_iternext
    // tp_methods
    CircularBuffer_methods,
    // tp_members
    CircularBuffer_members,
    0,  // tp_getset
    0,  // tp_base
    0,  // tp_dict
    0,  // tp_descr_get
    0,  // tp_descr_set
    0,  // tp_dictoffset
    // tp_init
    (initproc) CircularBuffer_initialize,
    0,  // tp_alloc
    // tp_new
    CircularBuffer_create,
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
