#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include "cursor.h"

// --------------------------------------------------------------------------

static inline bool
check_closed(cursor *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_ValueError, "I/O operation on closed file.");
        return true;
    }
    return false;
}

static int
check_writable(cursor *self)
{
    if (self->readonly) {
        PyObject* err = PyCursor_getunsupportedoperation();
        if (err != NULL)
            PyErr_SetString(err, "not writable");
        return true;
    }
    return false;
}

// --------------------------------------------------------------------------

static bool
_convert_iter(PyObject* obj, PyObject** it)
{
    PyObject* tmp = PyObject_GetIter(obj);
    if (tmp == NULL)
        return false;

    *it = tmp;
    return true;
}

static bool
_convert_optional_size(PyObject* obj, Py_ssize_t* s)
{
    if (obj == Py_None)
        return true;

    PyObject* n = PyNumber_Index(obj);
    if (n == NULL)
        return false;

    Py_ssize_t tmp = PyLong_AsSsize_t(n);
    Py_DECREF(n);

    if (PyErr_Occurred())
        return false;

    *s = tmp;
    return true;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_close___doc__,
  "close(self)\n"
  "--\n"
  "\n"
  "Disable all I/O operations.\n"
  "\n"
  "This method will effectively release the view on the buffer memory\n"
  "wrapped by the cursor.\n"
);

static PyObject*
iocursor_cursor_Cursor_close_impl(cursor* self)
{
    if (!self->closed) {
        PyBuffer_Release(&self->buffer);
        self->closed = true;
    }
    Py_RETURN_NONE;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_detach___doc__,
  "detach(self)\n"
  "--\n"
  "\n"
  "Disconnect the wrapper from its underlying raw stream.\n"
  "\n"
  "This is not supported by `Cursor` instances, since they do not wrap\n"
  "a raw stream, and calling this method will effectively raise the\n"
  "`io.UnsupportedOperation` exception.\n"
);

static PyObject*
iocursor_cursor_Cursor_detach_impl(cursor* self)
{
    PyObject* err = PyCursor_getunsupportedoperation();
    if (err != NULL)
        PyErr_SetString(err, "detach");
    return NULL;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_fileno___doc__,
  "fileno(self)\n"
  "--\n"
  "\n"
  "Return the underlying file descriptor if one exists.\n"
  "\n"
  "This is not supported by `Cursor` instances, since they do not wrap\n"
  "a file descriptor, and calling this method will effectively raise\n"
  "the `io.UnsupportedOperation` exception.\n"
);

static PyObject*
iocursor_cursor_Cursor_fileno_impl(cursor* self)
{
    PyObject* err = PyCursor_getunsupportedoperation();
    if (err != NULL)
        PyErr_SetString(err, "fileno");
    return NULL;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_flush___doc__,
  "flush(self)\n"
  "--\n"
  "\n"
  "Flush write buffers, if applicable.\n"
  "\n"
  "This is a no-op for `Cursor`, since they write directly to the\n"
  "provided buffer.\n"
);

static PyObject*
iocursor_cursor_Cursor_flush_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    Py_RETURN_NONE;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_getvalue___doc__,
  "getvalue(self)\n"
  "--\n"
  "\n"
  "Retrieve the backing memory buffer of the `Cursor` object.\n"
  "\n"
  "Example:\n"
  "    >>> cursor = Cursor(bytearray(4))\n"
  "    >>> cursor.write(b'abc')\n"
  "    3\n"
  "    >>> cursor.getvalue()\n"
  "    bytearray(b'abc\\x00')\n"
  "\n"
);

static PyObject*
iocursor_cursor_Cursor_getvalue_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    Py_INCREF(self->source);
    return self->source;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_isatty___doc__,
  "isatty(self)\n"
  "--\n"
  "\n"
  "Return whether the stream is attached to a TTY device.\n"
  "\n"
  "On `Cursor` instances, always return `False`.\n"
);

static PyObject*
iocursor_cursor_Cursor_isatty_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    Py_RETURN_FALSE;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_read___doc__,
  "read(self, size=-1)\n"
  "--\n"
  "\n"
  "Read at most ``size`` bytes, returned as a `bytes` object.\n"
  "\n"
  "An empty `bytes` object is returned at EOF.\n"
  "\n"
  "Arguments:\n"
  "    size (int, *optional*): The number of bytes to read. If\n"
  "        negative or `None`, read until EOF is reached.\n"
  "\n"
);

PyDoc_STRVAR(
  iocursor_cursor_Cursor_read1___doc__,
  "read1(self, size=-1)\n"
  "--\n"
  "\n"
  "Read at most ``size`` bytes with a single syscall.\n"
  "\n"
  "An empty `bytes` object is returned at EOF.\n"
  "\n"
  "Arguments:\n"
  "    size (int, *optional*): The number of bytes to read. If\n"
  "        negative or `None`, read until EOF is reached.\n"
  "\n"
);

static inline PyObject*
iocursor_cursor_Cursor_read_impl(cursor* self, Py_ssize_t size)
{
    if (check_closed(self))
        return NULL;

    if ((size == -1) || (self->offset >= self->buffer.len - size))
        size = self->buffer.len - self->offset;
    if (size < 0)
        size = 0;

    PyObject* bytes = PyBytes_FromStringAndSize(&((char*) self->buffer.buf)[self->offset], size);
    if (bytes == NULL) {
        return PyErr_NoMemory();
    }

    self->offset += size;
    return bytes;
}

static PyObject*
iocursor_cursor_Cursor_read(PyObject *self, PyObject *args, PyObject *kwargs)
{
    assert(Py_TYPE(self) == PyCursor_Type);

    PyObject *return_value = NULL;
    cursor* crs            = (cursor*) self;
    Py_ssize_t size        = -1;

    static char* keywords[] = {"size", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O&", keywords, &_convert_optional_size, &size)) {
        return_value = iocursor_cursor_Cursor_read_impl(crs, size);
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_readable___doc__,
  "readable(self)\n"
  "--\n"
  "\n"
  "Return ``True`` if the stream can be read from.\n"
  "\n"
  "On `Cursor` instances, this always returns `True`, since the\n"
  "buffers cannot be opened in write-only mode.\n"
);

static PyObject*
iocursor_cursor_Cursor_readable_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    Py_RETURN_TRUE;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_readinto___doc__,
  "readinto(self, buffer)\n"
  "--\n"
  "\n"
  "Read bytes into the provided buffer.\n"
  "\n"
  "Returns:\n"
  "    int: The number of bytes read, or 0 if the cursor is at EOF.\n"
  "\n"
);

PyDoc_STRVAR(
  iocursor_cursor_Cursor_readinto1___doc__,
  "readinto1(self, buffer)\n"
  "--\n"
  "\n"
  "Read bytes into the provided buffer with a single syscall.\n"
  "\n"
  "Returns:\n"
  "    int: The number of bytes read, or 0 if the cursor is at EOF.\n"
  "\n"
);

static inline PyObject*
iocursor_cursor_Cursor_readinto_impl(cursor* self, Py_buffer* buffer)
{
    Py_ssize_t nbytes = buffer->len;

    if (check_closed(self))
        return NULL;

    if (self->offset >= self->buffer.len)
        nbytes = 0;
    else if (nbytes > self->buffer.len - self->offset)
        nbytes = self->buffer.len - self->offset;

    memcpy(buffer->buf, &((char*) self->buffer.buf)[self->offset], nbytes);
    self->offset += nbytes;
    return PyLong_FromSsize_t(nbytes);
}

static PyObject*
iocursor_cursor_Cursor_readinto(PyObject *self, PyObject *args, PyObject *kwargs)
{
    assert(Py_TYPE(self) == PyCursor_Type);

    Py_buffer buffer;
    PyObject* return_value = NULL;
    cursor*   crs            = (cursor*) self;

    static char* keywords[] = {"buffer", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "w*", keywords, &buffer)) {
        return_value = iocursor_cursor_Cursor_readinto_impl(crs, &buffer);
        PyBuffer_Release(&buffer);
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_readline___doc__,
  "readline(self, size=-1)\n"
  "--\n"
  "\n"
  "Return the next line from the file, as a bytes object.\n"
  "\n"
);

static inline PyObject*
iocursor_cursor_Cursor_readline_impl(cursor* self, Py_ssize_t size) {
    if (check_closed(self))
        return NULL;

    if ((size < 0) || (size >= self->buffer.len - self->offset))
        size = (self->offset > self->buffer.len) ? 0 : self->buffer.len - self->offset;
    if (size == 0)
        return PyBytes_FromStringAndSize(NULL, 0);

    void* start = (void*) &((char*) self->buffer.buf)[self->offset];
    void* end   = memchr(start, '\n', size);

    Py_ssize_t length = (end == NULL) ? size : end - start + 1;
    PyObject* bytes = PyBytes_FromStringAndSize((char*) start, length);
    if (bytes == NULL)
        return PyErr_NoMemory();

    self->offset += length;
    return bytes;
}

static PyObject*
iocursor_cursor_Cursor_readline(PyObject* self, PyObject *args, PyObject *kwargs) {
    assert(Py_TYPE(self) == PyCursor_Type);

    PyObject* return_value = NULL;
    cursor* crs            = (cursor*) self;
    Py_ssize_t size        = -1;

    static char* keywords[] = {"size", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O&", keywords, &_convert_optional_size, &size)) {
        return_value = iocursor_cursor_Cursor_readline_impl(crs, size);
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_readlines___doc__,
  "readlines(self, hint=-1)\n"
  "--\n"
  "\n"
  "Collect all lines from the file into a `list` of `bytes`.\n"
  "\n"
  "Arguments:\n"
  "    hint (int, *optional*): An approximate bound on the total\n"
  "        number of bytes to be read. When positive, `readlines` will\n"
  "        stop reading lines when the total number of bytes read\n"
  "        exceeds ``hint``.\n"
  "\n"
  "Example:\n"
  "    >>> text = b'abc\\ndef\\nghi\\n'\n"
  "    >>> Cursor(text).readlines()\n"
  "    [b'abc\\n', b'def\\n', b'ghi\\n']\n"
  "    >>> Cursor(text).readlines(5)\n"
  "    [b'abc\\n', b'def\\n']\n"
  "\n"
);

static inline PyObject*
iocursor_cursor_Cursor_readlines_impl(cursor* self, Py_ssize_t hint) {

    void*      start;
    void*      end;
    PyObject*  bytes;
    PyObject*  lines;
    Py_ssize_t length = 0;
    Py_ssize_t total  = 0;
    Py_ssize_t size   = (self->offset > self->buffer.len) ? 0 : self->buffer.len - self->offset;

    if ((hint <= 0) || hint > size)
        hint = size;

    if (check_closed(self))
        return NULL;

    if ((lines = PyList_New(0)) == NULL)
        return PyErr_NoMemory();

    while (total < hint) {
        start = &((char*) self->buffer.buf)[self->offset];
        end   = memchr(start, '\n', size);

        length = (end == NULL) ? size : end - start + 1;
        if ((bytes = PyBytes_FromStringAndSize((char*) start, length)) == NULL) {
            Py_DECREF(lines);
            return PyErr_NoMemory();
        } else {
            PyList_Append(lines, bytes);
            Py_DECREF(bytes);
        }

        self->offset += length;
        size -= length;
        total += length;
    }

    return lines;
}

static PyObject*
iocursor_cursor_Cursor_readlines(PyObject* self, PyObject *args, PyObject *kwargs) {
    assert(Py_TYPE(self) == PyCursor_Type);

    PyObject* return_value = NULL;
    cursor* crs            = (cursor*) self;
    Py_ssize_t hint        = -1;

    static char* keywords[] = {"hint", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O&", keywords, &_convert_optional_size, &hint)) {
        return_value = iocursor_cursor_Cursor_readlines_impl(crs, hint);
    }

    return return_value;
}


// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_seek___doc__,
  "seek(self, pos, whence=0)\n"
  "--\n"
  "\n"
  "Change the stream position.\n"
  "\n"
  "Seek to byte offset pos relative to position indicated by whence:\n"
  "    - 0  Start of stream (the default) - pos should be >= 0.\n"
  "    - 1  Current position - pos may be negative.\n"
  "    - 2  End of stream - pos usually negative.\n"
  "\n"
  "Returns\n"
  "    int: The new absolute position.\n"
  "\n"
);

static inline PyObject*
iocursor_cursor_Cursor_seek_impl(cursor* self, Py_ssize_t pos, int whence)
{
    Py_ssize_t new_pos;

    if (check_closed(self))
        return NULL;

    switch (whence) {
        case SEEK_SET:
            new_pos = pos;
            break;
        case SEEK_CUR:
            if (pos > PY_SSIZE_T_MAX - self->offset) {
                PyErr_SetString(PyExc_OverflowError, "new position too large");
                return NULL;
            }
            new_pos = self->offset + pos;
            break;
        case SEEK_END:
            if (pos > PY_SSIZE_T_MAX - self->buffer.len) {
                PyErr_SetString(PyExc_OverflowError, "new position too large");
                return NULL;
            }
            new_pos = self->buffer.len + pos;
            break;
        default:
            return PyErr_Format(
                PyExc_ValueError,
                "invalid whence (%i, should be %i, %i or %i)",
                whence,
                SEEK_SET,
                SEEK_CUR,
                SEEK_END
            );
    }

    if (new_pos < 0) {
        if (whence == SEEK_SET) {
            PyErr_Format(PyExc_ValueError, "negative seek value %zd", pos);
            return NULL;
        } else {
            new_pos = 0;
        }
    }

    self->offset = new_pos;
    return PyLong_FromSsize_t(new_pos);
}

static PyObject*
iocursor_cursor_Cursor_seek(PyObject *self, PyObject *args, PyObject *kwargs)
{
    assert(Py_TYPE(self) == PyCursor_Type);

    PyObject* return_value = NULL;
    cursor* crs            = (cursor*) self;
    Py_ssize_t pos         = 0;
    int whence             = SEEK_SET;

    static char* keywords[] = {"pos", "whence", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "n|i", keywords, &pos, &whence)) {
        return_value = iocursor_cursor_Cursor_seek_impl(crs, pos, whence);
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_seekable___doc__,
  "seekable(self)\n"
  "--\n"
  "\n"
  "Return `True` if the stream supports random access.\n"
  "\n"
  "Always `True` for `Cursor` instances.\n"
  "\n"
);

static PyObject*
iocursor_cursor_Cursor_seekable_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    Py_RETURN_TRUE;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_tell___doc__,
  "tell(self)\n"
  "--\n"
  "\n"
  "Get the current file position as an integer.\n"
  "\n"
);

static PyObject*
iocursor_cursor_Cursor_tell_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    return PyLong_FromSsize_t(self->offset);
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_truncate___doc__,
  "truncate(self, size=None, /)\n"
  "--\n"
  "\n"
  "Truncate the file to at most size bytes.\n"
  "\n"
  "This is not supported by `Cursor` instances, and calling this\n"
  "method will effectively raise the `io.UnsupportedOperation`\n"
  "exception.\n"
  "\n"
);

static PyObject*
iocursor_cursor_Cursor_truncate_impl(cursor* self, Py_ssize_t size)
{
    if (check_closed(self))
        return NULL;

    PyObject* err = PyCursor_getunsupportedoperation();
    if (err != NULL)
        PyErr_SetString(err, "truncate");

    return NULL;
}

static PyObject*
iocursor_cursor_Cursor_truncate(PyObject *self, PyObject *args, PyObject *kwargs)
{
    assert(Py_TYPE(self) == PyCursor_Type);

    PyObject*  return_value   = NULL;
    cursor*    crs            = (cursor*) self;
    Py_ssize_t size           = crs->offset;

    static char* keywords[] = {"b", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O&", keywords, &_convert_optional_size, &size)) {
        return_value = iocursor_cursor_Cursor_truncate_impl(crs, size);
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_writable___doc__,
  "writable(self)\n"
  "--\n"
  "\n"
  "Return ``True`` if the stream supports writing to it."
);

static PyObject*
iocursor_cursor_Cursor_writable_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    return PyBool_FromLong(!(self->readonly || self->buffer.readonly));
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_write___doc__,
  "write(self, b, /)\n"
  "--\n"
  "\n"
  "Write the given bytes to the buffer.\n"
  "\n"
  "Returns:\n"
  "    int: The number of bytes written.\n"
  "\n"
);

static inline PyObject*
iocursor_cursor_Cursor_write_impl(cursor* self, Py_buffer* bytes)
{
    /* Check the cursor is still writable */
    if (check_closed(self))
        return NULL;
    if (check_writable(self))
        return NULL;

    /* No-op if there are no bytes to write */
    if (bytes->len > 0) {
        /* Check the buffer is large enough to hold the data */
        if ((self->offset >= self->buffer.len) || (bytes->len > self->buffer.len - self->offset)) {
            PyErr_Format(
                PyExc_BufferError,
                "cannot write %zd bytes to buffer of size %zd at position %zd",
                bytes->len,
                self->buffer.len,
                self->offset
            );
            return NULL;
        }
        /* Copy data from `bytes` to the buffer */
        memcpy(&((char*) self->buffer.buf)[self->offset], bytes->buf, bytes->len);
        self->offset += bytes->len;
    }

    return PyLong_FromSsize_t(bytes->len);
}

static PyObject*
iocursor_cursor_Cursor_write(PyObject *self, PyObject *args, PyObject *kwargs)
{
    assert(Py_TYPE(self) == PyCursor_Type);

    Py_buffer bytes;
    PyObject *return_value = NULL;
    cursor* crs            = (cursor*) self;

    static char* keywords[] = {"b", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "y*", keywords, &bytes)) {
        return_value = iocursor_cursor_Cursor_write_impl(crs, &bytes);
        PyBuffer_Release(&bytes);
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_writelines___doc__,
  "writelines(self, lines, /)\n"
  "--\n"
  "\n"
  "Write a list of lines to the buffer.\n"
  "\n"
);

static inline PyObject*
iocursor_cursor_Cursor_writelines_impl(cursor* self, PyObject* it)
{
    PyObject* item;
    Py_buffer line;

    /* Check the cursor is still writable */
    if (check_closed(self))
        return NULL;
    if (check_writable(self))
        return NULL;

    while ((item = PyIter_Next(it))) {
        /* Extract the item to a buffer */
        if (PyObject_GetBuffer(item, &line, PyBUF_SIMPLE) < 0) {
            Py_DECREF(item);
            return NULL;
        }

        /* Check we can write the entirety of the line to the buffer */
        if ((self->offset >= self->buffer.len) || (line.len > self->buffer.len - self->offset)) {
            PyErr_Format(
                PyExc_BufferError,
                "cannot write %zd bytes to buffer of size %zd at position %zd",
                line.len,
                self->buffer.len,
                self->offset
            );
            PyBuffer_Release(&line);
            Py_DECREF(item);
            return NULL;
        }

        /* Write the line to the buffer */
        memcpy(&((char*) self->buffer.buf)[self->offset], line.buf, line.len);
        self->offset += line.len;

        /* release buffer and reference when done */
        PyBuffer_Release(&line);
        Py_DECREF(item);
    }

    if (PyErr_Occurred())
        return NULL;

    Py_RETURN_NONE;
}

static PyObject*
iocursor_cursor_Cursor_writelines(PyObject *self, PyObject *args, PyObject *kwargs)
{
    assert(Py_TYPE(self) == PyCursor_Type);

    PyObject* it;
    PyObject* return_value = NULL;
    cursor*   crs          = (cursor*) self;

    static char* keywords[] = {"b", NULL};
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O&", keywords, &_convert_iter, &it)) {
        return_value = iocursor_cursor_Cursor_writelines_impl(crs, it);
        Py_DECREF(it);
    }

    return return_value;
}

// --------------------------------------------------------------------------

static PyObject *
iocursor_cursor_Cursor___new__(PyTypeObject* type, PyObject* args, PyObject* kwds)
{
    cursor *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = (cursor*) type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->buffer.obj = NULL;
    self->readonly = false;
    self->closed = false;
    self->offset = 0;
    self->source = NULL;

    return (PyObject *)self;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor___init____doc__,
  "\n"
  "A buffered I/O implementation wrapping a bytes buffer."
  "\n"
);

static inline int
iocursor_cursor_Cursor___init___impl(cursor* self, PyObject* source, bool readonly)
{
    int return_value = 0;

    /* Allow calling __init__ more than once, in that case make sure to
       release any previous object reference */
    self->offset = 0;
    if (self->buffer.buf != NULL)
        PyBuffer_Release(&self->buffer);
    self->buffer.buf = NULL;
    Py_XDECREF(self->source);

    /* Register the source object */
    self->source = source;
    Py_INCREF(source);

    /* Mark the cursor as 'open' */
    self->closed = false;
    self->readonly = false;

    /* Get a buffer for the source object */
    if (!readonly) {
        return_value = PyObject_GetBuffer(source, &self->buffer, PyBUF_SIMPLE | PyBUF_WRITABLE);
        if (return_value < 0) {
            PyErr_Clear();
            readonly = true;
            return_value = 0;
        }
    }

    if (self->buffer.buf == NULL) {
        return_value = PyObject_GetBuffer(source, &self->buffer, PyBUF_SIMPLE);
        self->readonly = true;
    }

    return return_value;
}

static int
iocursor_cursor_Cursor___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char* keywords[] = {"buffer", "readonly", NULL};

    PyObject* source   = NULL;
    int       readonly = false;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|p", keywords, &source, &readonly)) {
        return_value = iocursor_cursor_Cursor___init___impl(
            (cursor*) self,
            source,
            (bool) readonly
        );
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor___enter_____doc__,
  "__enter__(self)\n"
  "--\n"
  "\n"
  ""
);

static PyObject*
iocursor_cursor_Cursor___enter___impl(cursor* self)
{
    Py_INCREF(self);
    return (PyObject*) self;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor___exit_____doc__,
  "__exit__(self, exc_type, exc_value, traceback)\n"
  "--\n"
  "\n"
  ""
);

static PyObject*
iocursor_cursor_Cursor___exit___impl(cursor* self, PyObject* exc_type, PyObject* exc_value, PyObject* traceback)
{
    if (iocursor_cursor_Cursor_close_impl(self) == NULL)
        return NULL;
    Py_RETURN_FALSE;
}

static PyObject*
iocursor_cursor_Cursor___exit__(PyObject *self, PyObject *args, PyObject *kwargs)
{

    PyObject*    return_value = NULL;
    static char* keywords[]   = {"exc_type", "exc_value", "traceback"};

    PyObject* exc_type  = Py_None;
    PyObject* exc_value = Py_None;
    PyObject* traceback = Py_None;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|OOO", keywords, &exc_type, &exc_value, &traceback)) {
        return_value = iocursor_cursor_Cursor___exit___impl(
            (cursor*) self,
            exc_type,
            exc_value,
            traceback
        );
    }

    return return_value;
}

// --------------------------------------------------------------------------

static PyObject*
iocursor_cursor_Cursor___next___impl(cursor* self)
{
    if (self->offset >= self->buffer.len)
        return NULL;
    return iocursor_cursor_Cursor_readline_impl(self, -1);
}

// --------------------------------------------------------------------------

static PyObject*
iocursor_cursor_Cursor___repr___impl(cursor* self)
{
    if (self->readonly && !self->buffer.readonly)
        return PyUnicode_FromFormat("Cursor(\%R, readonly=True)", self->source);
    else
        return PyUnicode_FromFormat("Cursor(\%R)", self->source);
}

// --------------------------------------------------------------------------

static int
cursor_clear(cursor *self)
{
    Py_CLEAR(self->source);
    return 0;
}

static void
cursor_dealloc(cursor *self)
{
    if (!self->closed) {
        self->closed = true;
        PyBuffer_Release(&self->buffer);
    }
    PyObject_GC_UnTrack(self);
    Py_CLEAR(self->source);
    Py_TYPE(self)->tp_free(self);
}

static int
cursor_traverse(cursor* self, visitproc visit, void* arg)
{
    Py_VISIT(self->source);
    return 0;
}

// --------------------------------------------------------------------------

static struct PyMemberDef cursor_members[] = {
    {"closed",   T_BOOL, offsetof(cursor, closed),   READONLY, NULL},
    {NULL}  /* Sentinel */
};

static struct PyMethodDef cursor_methods[] = {
    {"__enter__",  (PyCFunction)                          iocursor_cursor_Cursor___enter___impl, METH_NOARGS,                  iocursor_cursor_Cursor___enter_____doc__},
    {"__exit__",   (PyCFunction)                          iocursor_cursor_Cursor___exit__,       METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor___exit_____doc__},
    {"close",      (PyCFunction)                          iocursor_cursor_Cursor_close_impl,     METH_NOARGS,                  iocursor_cursor_Cursor_close___doc__},
    {"detach",     (PyCFunction)                          iocursor_cursor_Cursor_detach_impl,    METH_NOARGS,                  iocursor_cursor_Cursor_detach___doc__},
    {"fileno",     (PyCFunction)                          iocursor_cursor_Cursor_fileno_impl,    METH_NOARGS,                  iocursor_cursor_Cursor_fileno___doc__},
    {"flush",      (PyCFunction)                          iocursor_cursor_Cursor_flush_impl,     METH_NOARGS,                  iocursor_cursor_Cursor_flush___doc__},
    {"getvalue",   (PyCFunction)                          iocursor_cursor_Cursor_getvalue_impl,  METH_NOARGS,                  iocursor_cursor_Cursor_getvalue___doc__},
    {"isatty",     (PyCFunction)                          iocursor_cursor_Cursor_isatty_impl,    METH_NOARGS,                  iocursor_cursor_Cursor_isatty___doc__},
    {"read",       (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_read,           METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_read___doc__},
    {"read1",      (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_read,           METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_read1___doc__},
    {"readable",   (PyCFunction)                          iocursor_cursor_Cursor_readable_impl,  METH_NOARGS,                  iocursor_cursor_Cursor_readable___doc__},
    {"readinto",   (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_readinto,       METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_readinto___doc__},
    {"readinto1",  (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_readinto,       METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_readinto1___doc__},
    {"readline",   (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_readline,       METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_readline___doc__},
    {"readlines",  (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_readlines,      METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_readlines___doc__},
    {"seek",       (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_seek,           METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_seek___doc__},
    {"seekable",   (PyCFunction)                          iocursor_cursor_Cursor_seekable_impl,  METH_NOARGS,                  iocursor_cursor_Cursor_seekable___doc__},
    {"tell",       (PyCFunction)                          iocursor_cursor_Cursor_tell_impl,      METH_NOARGS,                  iocursor_cursor_Cursor_tell___doc__},
    {"truncate",   (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_truncate,       METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_truncate___doc__},
    {"writable",   (PyCFunction)                          iocursor_cursor_Cursor_writable_impl,  METH_NOARGS,                  iocursor_cursor_Cursor_writable___doc__},
    {"write",      (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_write,          METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_write___doc__},
    {"writelines", (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_writelines,     METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_writelines___doc__},
    {NULL, NULL}  /* sentinel */
};

PyTypeObject PyCursor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "iocursor.cursor.Cursor",
    .tp_basicsize = sizeof(cursor),
    .tp_dealloc   = (destructor) cursor_dealloc,
    .tp_repr      = (reprfunc) iocursor_cursor_Cursor___repr___impl,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_doc       = iocursor_cursor_Cursor___init____doc__,
    .tp_traverse  = (traverseproc) cursor_traverse,
    .tp_clear     = (inquiry) cursor_clear,
    .tp_iter      = PyObject_SelfIter,
    .tp_iternext  = (iternextfunc) iocursor_cursor_Cursor___next___impl,
    .tp_methods   = cursor_methods,
    .tp_members   = cursor_members,
    .tp_init      = iocursor_cursor_Cursor___init__,
    .tp_new       = iocursor_cursor_Cursor___new__,
};

// --- cursor module ---------------------------------------------------------

static inline PyCursor_State*
cursormodule_getstate(PyObject *module) {
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (PyCursor_State *)state;
}

static int
cursormodule_traverse(PyObject *mod, visitproc visit, void *arg) {
    PyCursor_State* state = cursormodule_getstate(mod);
    if (!state->initialized)
        return 0;
    Py_VISIT(state->unsupported_operation);
    return 0;
}

static int
cursormodule_clear(PyObject *mod) {
    PyCursor_State* state = cursormodule_getstate(mod);
    if (!state->initialized)
        return 0;
    Py_CLEAR(state->unsupported_operation);
    return 0;
}

static void
cursormodule_free(PyObject *mod) {
    cursormodule_clear(mod);
}

static struct PyMethodDef cursormodule_methods[] = {
    {NULL, NULL}  /* sentinel */
};

static struct PyModuleDef PyCursor_Module = {
    PyModuleDef_HEAD_INIT,
    .m_name     = "cursor",
    .m_doc      = NULL,
    .m_size     = sizeof(PyCursor_State),
    .m_methods  = cursormodule_methods,
    .m_traverse = cursormodule_traverse,
    .m_clear    = cursormodule_clear,
    .m_free     = (freefunc)cursormodule_free,
};

PyMODINIT_FUNC
PyInit_cursor(void)
{
    PyObject* m           = NULL;
    PyObject* _io         = NULL;
    PyCursor_State* state = NULL;

    /* Create the module and initialize state */
    m = PyModule_Create(&PyCursor_Module);
    if (m == NULL)
        goto exit;
    state = cursormodule_getstate(m);
    state->initialized = 0;
    state->unsupported_operation = NULL;

    /* Add the `Cursor` class to the module */
    if (PyType_Ready(&PyCursor_Type) < 0)
        goto fail;
    if (PyModule_AddObject(m, "Cursor", (PyObject*) &PyCursor_Type) < 0)
        goto fail;

    /* Import the _io module and get the `UnsupportedOperation` exception */
    _io = PyImport_ImportModule("_io");
    if (_io == NULL)
        goto fail;
    state->unsupported_operation = PyObject_GetAttrString(_io, "UnsupportedOperation");
    if (state->unsupported_operation == NULL)
        goto fail;
    if (PyModule_AddObject(m, "UnsupportedOperation", state->unsupported_operation) < 0)
        goto fail;

    /* Increate the reference count for classes stored in the module */
    Py_INCREF(&PyCursor_Type);
    Py_INCREF(state->unsupported_operation);

exit:
    return m;

fail:
    Py_DECREF(m);
    Py_XDECREF(&PyCursor_Type);
    Py_XDECREF(state->unsupported_operation);
    return NULL;
}

// --- cursor module state ---------------------------------------------------

#ifdef CPYTHON

static PyCursor_State*
PyCursor_getstate(void)
{
    PyObject *mod = PyState_FindModule(&PyCursor_Module);
    PyCursor_State* state;
    if (mod == NULL || (state = cursormodule_getstate(mod)) == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "could not find io module state "
                        "(interpreter shutdown?)");
        return NULL;
    }
    return state;
}

static PyObject*
PyCursor_getunsupportedoperation(void)
{
    PyCursor_State* state = PyCursor_getstate();
    return (state == NULL) ? NULL : state->unsupported_operation;
}

#else

static PyObject*
PyCursor_getunsupportedoperation(void)
{
    PyObject* err = NULL;
    PyObject* _io = PyImport_ImportModule("_io");

    if (_io != NULL)
        err = PyObject_GetAttrString(_io, "UnsupportedOperation");

    return err;
}

#endif
