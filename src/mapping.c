#define PY_SSIZE_T_CLEAN
#include "mapping.h"
#include "sequence.h"

PyObject* CircularBuffer_get_subscript(CircularBuffer* self,
        PyObject* item)
{
    Py_ssize_t len = circularbuffer_total_length(self);
    Py_ssize_t pos;

    if (PyIndex_Check(item))
    {
        pos = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (pos == -1 && PyErr_Occurred())
        {
            return NULL;
        }
        else if (pos < 0)
        {
            pos += len;
        }
        return CircularBuffer_get_item(self, pos);
    }
    else if (PySlice_Check(item))
    {
        Py_ssize_t start, stop, step, slicelength, cur, i;

        if (PySlice_GetIndicesEx(item, len, &start, &stop, &step,
                &slicelength) < 0)
        {
            return NULL;
        }

        if (slicelength <= 0)
        {
            return Py_BuildValue(STR_FORMAT_BYTE, "", 0);
        }
        else if (step == 1)
        {
            return circularbuffer_peek_partial(self, start, stop);
        }
        else {
            PyObject *result;
            char* tempbuf = (char *)PyMem_Malloc(slicelength);

            if (tempbuf == NULL)
            {
                return PyErr_NoMemory();
            }

            for (cur = start, i = 0; i < slicelength; cur += step, i++)
            {
                pos = circularbuffer_translated_position(self, cur);
                tempbuf[i] = self->raw[pos];
            }

            result = Py_BuildValue(STR_FORMAT_BYTE, tempbuf, slicelength);

            PyMem_Free(tempbuf);
            return result;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "sequence index must be integer");
        return NULL;
    }
}


int CircularBuffer_set_subscript(CircularBuffer* self,
        PyObject* item, PyObject* value)
{
/*
    PyBufferProcs *pb;
    void *ptr1, *ptr2;
    Py_ssize_t selfsize;
    Py_ssize_t othersize;

    pb = value ? Py_TYPE(value)->tp_as_buffer : NULL;
    if ( pb == NULL ||
         pb->bf_getreadbuffer == NULL ||
         pb->bf_getsegcount == NULL )
    {
        PyErr_BadArgument();
        return -1;
    }
    if ( (*pb->bf_getsegcount)(value, NULL) != 1 )
    {
        // ### use a different exception type/message?
        PyErr_SetString(PyExc_TypeError,
                        "single-segment buffer object expected");
        return -1;
    }
    if (!get_buf(self, &ptr1, &selfsize, ANY_BUFFER))
        return -1;
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += selfsize;
        return buffer_ass_item(self, i, value);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength;

        if (PySlice_GetIndicesEx((PySliceObject *)item, selfsize,
                        &start, &stop, &step, &slicelength) < 0)
            return -1;

        if ((othersize = (*pb->bf_getreadbuffer)(value, 0, &ptr2)) < 0)
            return -1;

        if (othersize != slicelength) {
            PyErr_SetString(
                PyExc_TypeError,
                "right operand length must match slice length");
            return -1;
        }

        if (slicelength == 0)
            return 0;
        else if (step == 1) {
            memcpy((char *)ptr1 + start, ptr2, slicelength);
            return 0;
        }
        else {
            Py_ssize_t cur, i;

            for (cur = start, i = 0; i < slicelength;
                 cur += step, i++) {
                ((char *)ptr1)[cur] = ((char *)ptr2)[i];
            }

            return 0;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "buffer indices must be integers");
        return -1;
    }
*/
    PyErr_SetNone(PyExc_NotImplementedError);
    return -1;
}


PyMappingMethods CircularBuffer_mapping[] = {
    (lenfunc) CircularBuffer_length,                 // mp_length
    (binaryfunc) CircularBuffer_get_subscript,       // mp_subscript
    0,//(objobjargproc) CircularBuffer_set_subscript,    // mp_ass_subscript
};
