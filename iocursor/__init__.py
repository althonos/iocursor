# coding: utf-8
"""A zero-copy file-like wrapper for Python byte buffers.
"""

import io
import os

from .cursor import Cursor

__author__ = "Martin Larralde <martin.larralde@embl.de>"
__version__ = "0.1.1"
__license__ = "MIT"
__all__ = ["Cursor"]

io.IOBase.register(Cursor)  # type: ignore
io.BufferedIOBase.register(Cursor)  # type: ignore
