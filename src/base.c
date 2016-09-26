#include "base.h"

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
Py_ssize_t circularbuffer_forward_length(CircularBuffer* self,
        Py_ssize_t start)
{
    if (self->write >= start)
    {
        return self->write - start;
    }
    else
    {
        return self->allocated_before_resize - start + 1;
    }
}


/*
 * Total size of stored data.
 */
Py_ssize_t circularbuffer_total_length(CircularBuffer* self)
{
    Py_ssize_t len = circularbuffer_forward_length(self, self->read);
    if (self->write < self->read)
    {
        len += circularbuffer_forward_length(self, 0);
    }
    return len;
}


/*
 * Size of sequential available storage.
 */
Py_ssize_t circularbuffer_forward_available(CircularBuffer* self)
{
    if (self->write == self->allocated + 1)
    {
        return self->read ? self->read - 1 : 0;
    }
    else if (self->write < self->read)
    {
        return self->read - self->write - 1;
    }
    else
    {
        return self->allocated - self->write + 1;
    }
}


/*
 * Total available storage.
 */
Py_ssize_t circularbuffer_total_available(CircularBuffer* self)
{
    if (self->write < self->read)
    {
        return self->read - self->write - 1;
    }
    else
    {
        return self->allocated - (self->write - self->read);
    }
}


/*
 * Actual position in our circular bufer.
 */
Py_ssize_t circularbuffer_translated_position(CircularBuffer* self,
        Py_ssize_t pos)
{
    if (pos < 0)
    {
        return pos;
    }
    else if (pos > circularbuffer_total_length(self))
    {
        return -1;
    }
    Py_ssize_t translated_pos = self->read + pos;
    if (translated_pos > self->allocated_before_resize)
    {
        translated_pos -= self->allocated_before_resize;
    }
    return translated_pos;
}


/*
 * Get partial content.
 * May alter internal buffer during the course of the function.
 */
PyObject* circularbuffer_peek_partial(CircularBuffer* self,
        Py_ssize_t start, Py_ssize_t size)
{
    PyObject* result;
    Py_ssize_t len = circularbuffer_total_length(self);
    if (size > len)
    {
        size = len;
    }
    Py_ssize_t pos_start = self->read + start;
    if (pos_start > self->allocated_before_resize)
    {
        pos_start -= self->allocated_before_resize;
    }
    Py_ssize_t pos_end = size > 0 ? pos_start + size - 1 : pos_start;
    if (pos_end > self->allocated_before_resize)
    {
        pos_end -= self->allocated_before_resize;
    }

    if (pos_end >= pos_start)
    {
        result = Py_BuildValue(STR_FORMAT_BYTE "#", &self->raw[pos_start],
                 size);
    }
    else
    {
        char tmp_char = self->raw[pos_end];
        self->raw[pos_end] = 0;

        result = PyBytes_FromFormat("%s%s", &self->raw[pos_start], self->raw);
        self->raw[pos_end] = tmp_char;
    }
    return result;
}


/*
 * Make the internal buffer's data contiguous, from two segments into one.
 */
int circularbuffer_make_contiguous(CircularBuffer* self)
{
    if (self->write >= self->read)
    {
        return 0;
    }
    else if (self->write_lock || self->read_write_lock)
    {
        // trying to reallign internal buffer while it was being used
        PyErr_SetString(ReservedError, "The internal buffer cannot be modified "
                "at the moment.");

        return -1;
    }
    self->read_lock++;
    self->read_write_lock++;
    self->write_lock++;

    // temporary storage, allocate half of the allocated
    Py_ssize_t half_size = (self->allocated_before_resize - 1) / 2 + 1;
    Py_ssize_t size = self->write < half_size ? self->write : half_size;

    char* tmp_raw = PyMem_Malloc(size);
    if (tmp_raw == NULL)
    {
        PyErr_NoMemory();
        self->write_lock--;
        self->read_write_lock--;
        self->read_lock--;
        return -1;
    }
    memcpy(tmp_raw, self->raw, size);

    // copy the first segment
    char* write_ptr = self->raw;
    size = circularbuffer_forward_length(self, self->read);
    memmove(write_ptr, &self->raw[self->read], size);
    write_ptr += size;

    // copy the last segment (if the second segment was larger then half)
    if (self->write > half_size)
    {
        size = circularbuffer_forward_length(self, half_size);
        memcpy(write_ptr + half_size, self->raw + half_size, size);
        size = half_size;
    }
    else
    {
        size = circularbuffer_forward_length(self, 0);
    }

    // copy the second segment
    memcpy(write_ptr, tmp_raw, size);
    PyMem_Free(tmp_raw);

    size = circularbuffer_total_length(self);
    self->raw[size] = 0;
    self->write = size;
    self->read = 0;
    self->allocated_before_resize = self->allocated;

    self->write_lock--;
    self->read_write_lock--;
    self->read_lock--;
    return 0;
}
