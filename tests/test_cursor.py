# coding: utf-8

import array
import io
import os
import sys
import unittest

# import numpy
from iocursor import Cursor


class TestReadCursorMixin:

    def test_interface(self):
        cursor = Cursor(self.make_buffer(b"abcd"))
        self.assertIsInstance(cursor, io.IOBase)
        self.assertIsInstance(cursor, io.BufferedIOBase)

    def test_repr_readonly(self):
        buffer = self.make_buffer(b"abcd")
        cursor = Cursor(buffer, readonly=True)
        self.assertEqual(repr(cursor), "Cursor({!r})".format(buffer))

    def test_read(self):
        cursor = Cursor(self.make_buffer(b"abcdefghijkl"))
        self.assertEqual(cursor.read(2), b"ab")
        self.assertEqual(cursor.read(3), b"cde")
        self.assertEqual(cursor.read(0), b"")
        self.assertEqual(cursor.read(), b"fghijkl")
        self.assertEqual(cursor.read(2), b"")
        self.assertEqual(cursor.read(), b"")

    def test_close(self):
        cursor = Cursor(self.make_buffer(b"abcd"))
        self.assertFalse(cursor.closed)
        cursor.close()
        self.assertTrue(cursor.closed)
        cursor.close()
        self.assertTrue(cursor.closed)

    def test_tell(self):
        cursor = Cursor(self.make_buffer(b"abcdefghijkl"))
        self.assertEqual(cursor.tell(), 0)
        cursor.read(4)
        self.assertEqual(cursor.tell(), 4)
        cursor.read()
        self.assertEqual(cursor.tell(), 12)

    def test_flags(self):
        buf = self.make_buffer(b"abc")
        cursor = Cursor(buf)
        self.assertEqual(cursor.writable(), False)
        self.assertEqual(cursor.readable(), True)
        self.assertEqual(cursor.seekable(), True)
        self.assertEqual(cursor.isatty(), False)
        self.assertEqual(cursor.closed, False)

        buf = self.make_buffer(b"abc")
        cursor = Cursor(buf, readonly=True)
        self.assertEqual(cursor.writable(), False)
        self.assertEqual(cursor.readable(), True)
        self.assertEqual(cursor.seekable(), True)
        self.assertEqual(cursor.isatty(), False)
        self.assertEqual(cursor.closed, False)

    def test_readonly(self):
        buffer = self.make_buffer(b"abc")
        cursor = Cursor(buffer, readonly=True)
        self.assertFalse(cursor.writable())
        self.assertRaises(io.UnsupportedOperation, cursor.write, b"blah\n")
        self.assertRaises(io.UnsupportedOperation, cursor.writelines, [b"blah\n"])

    def test_getvalue(self):
        buffer = self.make_buffer(b"abc")
        cursor = Cursor(buffer)
        self.assertIs(cursor.getvalue(), buffer)
        self.assertIsInstance(cursor.getvalue(), type(buffer))

    def test_seek_overflow(self):
        buffer = self.make_buffer(b"abcd")
        cursor = Cursor(buffer, readonly=True)
        self.assertEqual(cursor.seek(2), 2)
        self.assertRaises(OverflowError, cursor.seek, sys.maxsize - 1, whence=os.SEEK_CUR)
        self.assertRaises(OverflowError, cursor.seek, sys.maxsize - 1, whence=os.SEEK_END)


class TestWriteCursorMixin(TestReadCursorMixin):

    def test_flags(self):
        buf = self.make_buffer(b"abc")
        cursor = Cursor(buf)
        self.assertEqual(cursor.writable(), True)
        self.assertEqual(cursor.readable(), True)
        self.assertEqual(cursor.seekable(), True)
        self.assertEqual(cursor.isatty(), False)
        self.assertEqual(cursor.closed, False)

        buf = self.make_buffer(b"abc")
        cursor = Cursor(buf, readonly=True)
        self.assertEqual(cursor.writable(), False)
        self.assertEqual(cursor.readable(), True)
        self.assertEqual(cursor.seekable(), True)
        self.assertEqual(cursor.isatty(), False)
        self.assertEqual(cursor.closed, False)

    def test_repr_readonly(self):
        buffer = self.make_buffer(b"abcd")
        cursor = Cursor(buffer, readonly=True)
        self.assertEqual(repr(cursor), "Cursor({!r}, readonly=True)".format(buffer))

    def test_repr_write(self):
        buffer = self.make_buffer(b"abcd")
        cursor = Cursor(buffer)
        self.assertEqual(repr(cursor), "Cursor({!r})".format(buffer))

    def test_write(self):
        buffer = self.make_buffer(bytearray(256))
        cursor = Cursor(buffer)

        self.assertEqual(cursor.write(b"blah."), 5)
        self.assertEqual(cursor.seek(0), 0)
        self.assertEqual(cursor.read(5), b"blah.")
        self.assertEqual(cursor.seek(0), 0)
        self.assertEqual(cursor.write(b"Hello."), 6)
        self.assertEqual(cursor.tell(), 6)
        self.assertEqual(cursor.seek(5), 5)
        self.assertEqual(cursor.tell(), 5)
        self.assertEqual(cursor.write(b" world\n\n\n"), 9)
        self.assertEqual(cursor.seek(0), 0)
        self.assertEqual(cursor.write(b"h"), 1)
        self.assertRaises(io.UnsupportedOperation, cursor.truncate, 12)
        self.assertEqual(cursor.tell(), 1)

        value = cursor.getvalue()
        self.assertIs(value, buffer)
        self.assertEqual(value[:12], self.make_buffer(b"hello world\n"))

    def test_write_overflow(self):
        buffer = self.make_buffer(bytearray(10))
        cursor = Cursor(buffer)

        self.assertEqual(cursor.write(b"0123456789"), 10)
        self.assertEqual(cursor.write(b""), 0)
        self.assertRaises(BufferError, cursor.write, b"abc")
        cursor.seek(-2, whence=os.SEEK_END)
        self.assertRaises(BufferError, cursor.write, b"abc")
        cursor.seek(-3, whence=os.SEEK_END)
        self.assertEqual(cursor.write(b"abc"), 3)
        self.assertEqual(bytes(buffer), b"0123456abc")

    def test_writelines_overflow(self):
        buffer = self.make_buffer(bytearray(8))
        cursor = Cursor(buffer)

        cursor.writelines([b"abc\n", b"def\n"])
        self.assertEqual(bytes(buffer), b"abc\ndef\n")
        self.assertEqual(cursor.seek(0), 0)
        self.assertRaises(BufferError, cursor.writelines, [b"123\n", b"456\n", b"789\n"])
        self.assertEqual(bytes(buffer), b"123\n456\n")


class TestCursorBytesMemoryview(unittest.TestCase, TestReadCursorMixin):

    @staticmethod
    def make_buffer(b):
        return memoryview(bytes(b))


class TestCursorBytes(unittest.TestCase, TestReadCursorMixin):

    @staticmethod
    def make_buffer(b):
        return bytes(b)


class TestCursorBytearray(unittest.TestCase, TestWriteCursorMixin):

    @staticmethod
    def make_buffer(b):
        return bytearray(b)


class TestCursorArray(unittest.TestCase, TestWriteCursorMixin):

    @staticmethod
    def make_buffer(b):
        return array.array('b', b)


# class TestCursorNumpyArray(unittest.TestCase, TestReadCursorMixin, TestWriteCursorMixin):
#
#     @staticmethod
#     def make_buffer(b):
#         return numpy.array(list(b), dtype="b")
#
#     def assertEqual(self, first, second, *args, **kwargs):
#         if isinstance(first, numpy.ndarray):
#             self.assertTrue(numpy.all(first == second), *args, **kwargs)
#         else:
#             super().assertEqual(first, second, *args, **kwargs)
