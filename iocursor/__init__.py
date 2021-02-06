# coding: utf-8
"""A zero-copy file-like wrapper for Python byte buffers.
"""

import io
import os

from .cursor import Cursor

__author__ = "Martin Larralde <martin.larralde@embl.de>"
__version__ = "0.1.0"
__license__ = "MIT"

io.IOBase.register(Cursor)  # type: ignore
io.BufferedIOBase.register(Cursor)  # type: ignore
