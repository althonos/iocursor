#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

// --------------------------------------------------------------------------

typedef struct {
    PyObject_HEAD
    bool        closed;
    Py_ssize_t  offset; /* the current position of the cursor in the file */
    PyObject*   source; /* the object the cursor was created to wrap */
    Py_buffer   buffer; /* an exported buffer view of the source object */
    PyObject*   mode;   /* the mode the cursor was created with */
} cursor;

// --------------------------------------------------------------------------

static int
check_closed(cursor *self)
{
    if (self->closed) {
        PyErr_SetString(PyExc_ValueError, "I/O operation on closed file.");
        return 1;
    }
    return 0;
}

static int
cursor_clear(cursor *self)
{
    if (!self->closed) {
        self->closed = true;
        PyBuffer_Release(&self->buffer);
    }

    Py_CLEAR(self->mode);
    Py_CLEAR(self->source);

    return 0;
}

static void
cursor_dealloc(cursor *self)
{
    PyObject_GC_UnTrack(self);

    Py_CLEAR(self->mode);
    Py_CLEAR(self->source);

    Py_TYPE(self)->tp_free(self);
}

static int
cursor_traverse(cursor* self, visitproc visit, void* arg)
{
    Py_VISIT(self->mode);
    Py_VISIT(self->source);
    return 0;
}

// --------------------------------------------------------------------------

static PyObject *
iocursor_cursor_Cursor___new__(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    cursor *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = (cursor*) type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->buffer.obj = NULL;
    self->closed = false;
    self->offset = 0;
    self->mode = NULL;
    self->source = NULL;

    return (PyObject *)self;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor___init____doc__,
  "Cursor(buffer, mode=\"\")\n"
  "--\n"
  "\n"
  "Buffered I/O implementation using an in-memory bytes buffer."
);

static int
iocursor_cursor_Cursor___init___impl(cursor* self, PyObject* source, PyObject* mode)
{
    /* Allow calling __init__ more than once, in that case make sure to
       release any previous object reference */
    self->offset = 0;

    /* Register the source object and the mode */
    self->source = source;
    Py_INCREF(source);

    /* Register a mode or use the default reading mode */
    if (mode == NULL) {
        self->mode = PyUnicode_FromString("r");
    } else {
        self->mode = mode;
        Py_INCREF(mode);
    }

    /* Mark the cursor as 'open' */
    self->closed = false;

    /* Get a buffer for the source object */
    return PyObject_GetBuffer(source, &self->buffer, PyBUF_SIMPLE);
}

static int
iocursor_cursor_Cursor___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char* keywords[] = {"buffer", "mode", NULL};

    PyObject* source = NULL;
    PyObject* mode = NULL;

    if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|U", keywords, &source, &mode)) {
        return_value = iocursor_cursor_Cursor___init___impl(
            (cursor*) self,
            source,
            mode
        );
    }

    return return_value;
}

// --------------------------------------------------------------------------

static PyObject*
iocursor_cursor_Cursor___repr___impl(PyObject* self)
{
    PyObject* mode   = PyObject_GetAttrString(self, "mode");
    PyObject* source = PyObject_GetAttrString(self, "source");
    return PyUnicode_FromFormat("Cursor(\%R, mode=\%R)", source, mode);
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_close___doc__,
  "close(self)"
  "--\n"
  "\n"
  ""
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
  iocursor_cursor_Cursor_read___doc__,
  "read(self, size=-1)"
  "--\n"
  "\n"
  ""
);

static PyObject*
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
    if (PyArg_ParseTupleAndKeywords(args, kwargs, "|n", keywords, &size)) {
        return_value = iocursor_cursor_Cursor_read_impl(crs, size);
    }

    return return_value;
}

// --------------------------------------------------------------------------

PyDoc_STRVAR(
  iocursor_cursor_Cursor_readable___doc__,
  "readable(self)"
  "--\n"
  "\n"
  "Return ``True`` if the stream can be read from."
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
  iocursor_cursor_Cursor_seekable___doc__,
  "seekable(self)"
  "--\n"
  "\n"
  "Return ``True`` if the stream supports random access."
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
  iocursor_cursor_Cursor_seek___doc__,
  "seek(self, pos, whence=0)"
  "--\n"
  "\n"
  ""
);

static PyObject*
iocursor_cursor_Cursor_seek_impl(cursor* self, Py_ssize_t pos, int whence)
{
    Py_ssize_t new_pos;

    if (check_closed(self))
        return NULL;

    switch (whence) {
        case SEEK_SET:
            new_pos = pos;
            if (new_pos < 0) {
                PyErr_Format(PyExc_ValueError, "negative seek value %li", pos);
                return NULL;
            }
            break;
        case SEEK_CUR:
            if (pos > PY_SSIZE_T_MAX - self->offset) {
                PyErr_SetString(PyExc_OverflowError, "new position too large");
                return NULL;
            }
            new_pos = self->offset + pos;
            if (new_pos < 0)
                new_pos = 0;
            break;
        case SEEK_END:
            if (pos > PY_SSIZE_T_MAX - self->buffer.len) {
                PyErr_SetString(PyExc_OverflowError, "new position too large");
                return NULL;
            }
            new_pos = self->buffer.len + pos;
            if (new_pos < 0)
                new_pos = 0;
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
  iocursor_cursor_Cursor_tell___doc__,
  "tell(self)"
  "--\n"
  "\n"
  ""
);

static PyObject*
iocursor_cursor_Cursor_tell_impl(cursor* self)
{
    if (check_closed(self))
        return NULL;
    return PyLong_FromSsize_t(self->offset);
}

// --------------------------------------------------------------------------

static struct PyMemberDef cursor_members[] = {
    {"closed", T_BOOL,      offsetof(cursor, closed), READONLY, NULL},
    {"source", T_OBJECT_EX, offsetof(cursor, source), READONLY, NULL},
    {"buffer", T_OBJECT_EX, offsetof(cursor, source), READONLY, NULL},
    {"mode",   T_OBJECT_EX, offsetof(cursor, mode),   READONLY, NULL},
    {NULL}  /* Sentinel */
};

static struct PyMethodDef cursor_methods[] = {
    {"close",    (PyCFunction)                          iocursor_cursor_Cursor_close_impl,       METH_NOARGS,                  iocursor_cursor_Cursor_close___doc__},
    {"read",     (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_read,             METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_read___doc__},
    {"readable", (PyCFunction)                          iocursor_cursor_Cursor_readable_impl,    METH_NOARGS,                  iocursor_cursor_Cursor_readable___doc__},
    {"seek",     (PyCFunction)(PyCFunctionWithKeywords) iocursor_cursor_Cursor_seek,             METH_VARARGS | METH_KEYWORDS, iocursor_cursor_Cursor_seek___doc__},
    {"seekable", (PyCFunction)                          iocursor_cursor_Cursor_seekable_impl,    METH_NOARGS,                  iocursor_cursor_Cursor_seekable___doc__},
    {"tell",     (PyCFunction)                          iocursor_cursor_Cursor_tell_impl,        METH_NOARGS,                  iocursor_cursor_Cursor_tell___doc__},
    {NULL, NULL}  /* sentinel */
};

PyTypeObject PyCursor_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "iocursor.cursor.Cursor",
    .tp_basicsize = sizeof(cursor),
    .tp_dealloc   = (destructor)cursor_dealloc,
    .tp_repr      = iocursor_cursor_Cursor___repr___impl,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_doc       = iocursor_cursor_Cursor___init____doc__,
    .tp_traverse  = (traverseproc)cursor_traverse,
    .tp_clear     = (inquiry)cursor_clear,
    .tp_iter      = PyObject_SelfIter,
    .tp_methods   = cursor_methods,
    .tp_members   = cursor_members,
    .tp_init      = iocursor_cursor_Cursor___init__,
    .tp_new       = iocursor_cursor_Cursor___new__,
};

// --- cursor module --------------------------------------------------------

static struct PyMethodDef cursor_module_methods[] = {
    {NULL, NULL}  /* sentinel */
};

static struct PyModuleDef cursor_module = {
    PyModuleDef_HEAD_INIT,
    "cursor",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    cursor_module_methods,
    NULL
};

PyMODINIT_FUNC
PyInit_cursor(void)
{
    if (PyType_Ready(&PyCursor_Type) < 0)
        return NULL;

    PyObject* m = PyModule_Create(&cursor_module);
    if (m == NULL)
        return NULL;

    // PyObject* _io = PyImport_ImportModule("_io");
    // if (_io == NULL)
    //     return NULL;
    //
    // PyObject* _BufferedIOBase = PyObject_GetAttrString(_io, "_BufferedIOBase");
    // if (_BufferedIOBase == NULL)
    //     return NULL:

    // PyCursor_Type.tp_base = (PyTypeObject*) _BufferedIOBase;

    Py_INCREF(&PyCursor_Type);
    if (PyModule_AddObject(m, "Cursor", (PyObject*) &PyCursor_Type) < 0) {
        Py_DECREF(&PyCursor_Type);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
