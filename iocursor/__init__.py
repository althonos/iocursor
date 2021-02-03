# coding: utf-8

import io
import os

from .cursor import Cursor

__version__ = "0.1.0"

io.IOBase.register(Cursor)
io.BufferedIOBase.register(Cursor)
