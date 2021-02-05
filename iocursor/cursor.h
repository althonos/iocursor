#ifndef _CURSOR_H
#define _CURSOR_H

typedef struct {
    PyObject_HEAD
    bool        closed;
    bool        readonly; /* whether the cursor is in read-only mode or not */
    Py_ssize_t  offset;   /* the current position of the cursor in the file */
    PyObject*   source;   /* the object the cursor was created to wrap */
    Py_buffer   buffer;   /* an exported buffer view of the source object */
} cursor;

typedef struct {
    int initialized;
    PyObject *unsupported_operation;
} PyCursor_State;

static PyCursor_State* PyCursor_getstate(void);
static PyObject* PyCursor_getunsupportedoperation(void);

#endif
