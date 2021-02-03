# coding: utf-8

import io
import os

from .cursor import Cursor

__version__ = "0.1.0"

io.IOBase.register(Cursor)
io.BufferedIOBase.register(Cursor)



#     def __init__(self, buffer, mode="r"):
#         self._buffer = buffer
#         self._memory = memoryview(buffer)
#         self._offset = len(buffer) * ("a" in mode)
#         self._mode = mode
#         self._lock = threading.RLock()
#
#         if self.writable() and self._memory.readonly:
#             raise ValueError("cannot open buffer {} in write mode".format(buffer))
#
#     # `io.IOBase` methods
#
#     def close(self):
#         self._memory.release()
#         super(Cursor, self).close()
#
#     def fileno(self):
#         raise io.UnsupportedOperatio("fileno")
#
#     def flush(self):
#         pass
#
#     def isatty(self):
#         return False
#
#     def readable(self):
#         return not set(self._mode).isdisjoint("r+")
#
#     def readline(self, size=-1):
#         raise NotImplementedError("readline")
#
#     def readlines(self, hint=-1):
#         raise NotImplementedError("readlines")
#
#     def seekable(self):
#         return True
#
#     def seek(offset, whence=os.SEEK_SET):
#         raise NotImplementedError("seek")
#
#     def tell(self):
#         return self._offset
#
#     def truncate(self):
#         for i in range(self._offset, len(self._buffer)):
#             self._buffer[i] = b"\0"
#
#     def writable(self):
#         return not set(self._mode).isdisjoint("wxa+")
#
#     def writelines(self, lines):
#         raise NotImplementedError("writelines")
#
#     # `io.BufferedIOBase` methods
#
#     def read1(self, size=None):
#         return self.read(size)
#
#     def readall(self):
#         raise NotImplementedError("readall")
#
#     def readinto(self, b):
#         raise NotImplementedError("readinto")
#
#     def readinto1(self, b):
#         return self.readinto(b)
#
#     def write(self, b):
#         raise NotImplementedError("write")
#
#     # `io.BytesIO` methods
#
#     def getbuffer(self):
#         return self._buffer
#
#     def getvalue(self):
#         return self._memory.tobytes()
