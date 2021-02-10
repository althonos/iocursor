# `io‸cursor`

*A [zero-copy](https://en.wikipedia.org/wiki/Zero-copy)
[file-like wrapper](https://docs.python.org/3/library/io.html#io.BufferedIOBase)
for [Python](https://www.python.org/) [byte buffers](https://docs.python.org/3/c-api/buffer.html),
inspired by Rust's [`std::io::Cursor`](https://doc.rust-lang.org/std/io/struct.Cursor.html).*

[![Actions](https://img.shields.io/github/workflow/status/althonos/iocursor/Test/master?logo=github&style=flat-square&maxAge=300)](https://github.com/althonos/iocursor/actions)
[![Coverage](https://img.shields.io/codecov/c/gh/althonos/iocursor?style=flat-square&maxAge=3600)](https://codecov.io/gh/althonos/iocursor/)
[![PyPI](https://img.shields.io/pypi/v/iocursor.svg?style=flat-square&maxAge=3600)](https://pypi.org/project/iocursor)
[![Wheel](https://img.shields.io/pypi/wheel/iocursor.svg?style=flat-square&maxAge=3600)](https://pypi.org/project/iocursor/#files)
[![Python Versions](https://img.shields.io/pypi/pyversions/iocursor.svg?style=flat-square&maxAge=3600)](https://pypi.org/project/iocursor/#files)
[![Python Implementations](https://img.shields.io/pypi/implementation/iocursor.svg?style=flat-square&maxAge=3600&label=impl)](https://pypi.org/project/iocursor/#files)
[![License](https://img.shields.io/badge/license-MIT-blue.svg?style=flat-square&maxAge=2678400)](https://choosealicense.com/licenses/mit/)
[![Source](https://img.shields.io/badge/source-GitHub-303030.svg?maxAge=2678400&style=flat-square)](https://github.com/althonos/iocursor/)
[![GitHub issues](https://img.shields.io/github/issues/althonos/iocursor.svg?style=flat-square&maxAge=600)](https://github.com/althonos/iocursor/issues)
[![Downloads](https://img.shields.io/badge/dynamic/json?style=flat-square&color=303f9f&maxAge=86400&label=downloads&query=%24.total_downloads&url=https%3A%2F%2Fapi.pepy.tech%2Fapi%2Fprojects%2Fiocursor)](https://pepy.tech/project/iocursor)
[![Changelog](https://img.shields.io/badge/keep%20a-changelog-8A0707.svg?maxAge=2678400&style=flat-square)](https://github.com/althonos/iocursor/blob/master/CHANGELOG.md)



## 🗺️ Overview

`iocursor.Cursor` lets you wrap an allocated buffer (i.e. a Python object
implementing the [buffer protocol](https://docs.python.org/3/c-api/buffer.html)),
and interfacing with it through the API of a file-like object. It shares
some common points with [`io.BytesIO`](https://docs.python.org/3/library/io.html#io.BytesIO)
but with the following main differences:

- *zero-copy* VS *copy*: `Cursor` will not copy the data you give it at
  initialisation, while `BytesIO` will. This makes `Cursor` more efficient
  when you are using it for read-only operations.
- *static* VS *growable*: `Cursor` will only use the buffer you give it at
  static memory, while `BytesIO` will use its dedicated, growable buffer.


## 🔧 Installing

Install directly from PyPI, using [pip](https://pip.pypa.io/):

```console
$ pip install iocursor
```

Pre-built wheels are available on Linux and OSX for all supported Python3
versions. Otherwise, building from source only requires a working C compiler.


## 🧶 Thread-safety

`iocursor.Cursor` instances are not thread-safe. Using several `Cursor`
instances with the same backend memory only for reading should be fine.
Use a lock when interfacing otherwise.


## 💡 Examples

- Use `iocursor.Cursor` when you have `bytes` you need to pass to an interface
  that only accepts file-like objects. For instance, pass a PNG image decoded
  from base64 to PIL, without copy:
  ```python
  import base64
  from iocursor import Cursor
  from PIL import Image

  imgdata = base64.b64decode("iVBORw0KGgoAAAANSUhEU...")
  img = Image.open(Cursor(imgdata))
  ```
- Use `iocursor.Cursor` when you want to use the file-like API to write
  to a buffer of known size. For instance, retrieve a file using the
  [`pysmb`](https://miketeo.net/blog/projects/pysmb) API, which only accepts
  file-like objects:
  ```python
  from SMB.SMBConnection import SMBConnectSMBConnection

  smb = SMBConnection('guest', '', 'client', 'server')
  smb.connect("192.168.0.1")

  info = smb.getAttributes("Music", "The Clash/Rock the Casbah.mp3")
  cursor = Cursor(bytearray(shared_file.file_size))
  smb.retrieveFile("Music", "The Clash/Rock the Casbah.mp3", cursor)

  buffer = cursor.getvalue()
  ```
- Use `iocursor.Cursor` when you want to do direct I/O on a type implementing
  the buffer protocol. For instance, initialize a [`numpy`](https://numpy.org/)
  [array](https://numpy.org/doc/stable/reference/arrays.html) by writing bytes
  to it:
  ```python
  import numpy

  array = numpy.empty(4, dtype="int16")
  cursor = Cursor(array)
  cursor.write(b"\x01\x00\x02\x00\x03\x00\x04\x00")
  print(array)  # array([1, 2, 3, 4], dtype=int16)
  ```


## 💭 Feedback

### ⚠️ Issue Tracker

Found a bug ? Have an enhancement request ? Head over to the [GitHub issue
tracker](https://github.com/althonos/iocursor/issues) if you need to report
or ask something. If you are filing in on a bug, please include as much
information as you can about the issue, and try to recreate the same bug
in a simple, easily reproducible situation.

### 🏗️ Contributing

Contributions are more than welcome! See [`CONTRIBUTING.md`](https://github.com/althonos/iocursor/blob/master/CONTRIBUTING.md) for more details.

## ⚖️ License

This library is provided under the [MIT License](https://choosealicense.com/licenses/mit/).
