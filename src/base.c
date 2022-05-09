#define PY_SSIZE_T_CLEAN
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
        Py_ssize_t start, Py_ssize_t end)
{
    Py_ssize_t len = circularbuffer_total_length(self);
    circularbuffer_parse_slice_notation(self, len, &start, &end);

    if (end <= start || start < 0 || end < 0)
    {
        return Py_BuildValue(STR_FORMAT_BYTE, "", 0);
    }

    len = end - start;

    start = circularbuffer_translated_position(self, start);
    Py_ssize_t avail = circularbuffer_forward_length(self, start);

    char *tmp = (char*) PyMem_Malloc(len);
    char *write = tmp;

    if (avail < len)
    {
        memcpy(write, &self->raw[start], avail);
        memcpy(write + avail, self->raw, len - avail);
    }
    else
    {
        memcpy(write, &self->raw[start], len);
    }

    PyObject *result = Py_BuildValue(STR_FORMAT_BYTE, tmp, len);
    PyMem_Free(tmp);
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


/*
 * Parse optional argument start and end coming from sequence like methods.
 */
void circularbuffer_parse_slice_notation(CircularBuffer *self, Py_ssize_t len,
        Py_ssize_t *start, Py_ssize_t *end)
{
    if (*start < 0)
    {
        *start = len + *start;
    }
    if (*end < 0)
    {
        *end = len + *end + 1;
    }
    else if (*end > len)
    {
        *end = len;
    }
}


/*
 * Get index of first match.
 */
Py_ssize_t circularbuffer_find(CircularBuffer *self, const char* search,
        Py_ssize_t search_len, Py_ssize_t start, Py_ssize_t end)
{
    Py_ssize_t len = circularbuffer_total_length(self);
    Py_ssize_t index = start + 1;

    circularbuffer_parse_slice_notation(self, len, &start, &end);

    if (end <= start || start < 0 || end < 0)
    {
        return -1;
    }

    start = circularbuffer_translated_position(self, start);
    end = circularbuffer_translated_position(self, end);

    const char* psearch = search;
    const char* psearch_end = psearch + search_len;

    const char* pread = &self->raw[start];
    const char* pread_half_end;
    if (end < start)
    {
        pread_half_end = self->raw + self->allocated_before_resize;
    }
    else
    {
        pread_half_end = self->raw + end;
    }

    while (pread < pread_half_end)
    {
        psearch = pread[0] == psearch[0] ? psearch + 1 : search;
        if (psearch == psearch_end)
        {
            return index - search_len;
        }
        pread += 1;
        index += 1;
    }
    if (end < start)
    {
        pread = self->raw;
        pread_half_end = self->raw + end;

        while (pread < pread_half_end)
        {
            psearch = pread[0] == psearch[0] ? psearch + 1 : search;
            if (psearch == psearch_end)
            {
                return index - search_len;
            }
            pread += 1;
            index += 1;
        }
    }

    return -1;
}
