# Contributing to `iocursor`

For bug fixes or new features, please file an issue before submitting a
pull request. If the change isn't trivial, it may be best to wait for
feedback.

## Running tests

Tests are written as usual Python unit tests with the `unittest` module of
the standard library. Running them requires the extension to be built
locally:

```console
$ python setup.py build_ext --debug --inplace
$ python -m unittest discover -vv
```

## Coding guidelines

This project targets Python 3.5 or later.

Python objects should be typed; since it is not supported in C extensions,
you must manually declare types in type stubs (`.pyi` files). In Python
files, you can add type annotations to function signatures (supported in
Python 3.5), but not in variable assignments (supported only from Python 3.6
onward).

### Interfacing with C

When interfacing with C, and in particular with pointers, try to use
debug assertions everywhere you assume the pointer to be non-NULL.
