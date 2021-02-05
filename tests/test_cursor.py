# coding: utf-8

import array
import io
import unittest

# import numpy
from iocursor import Cursor


class TestReadCursorMixin:

    def test_interface(self):
        cursor = Cursor(self.make_buffer(b"abcd"))
        self.assertIsInstance(cursor, io.IOBase)
        self.assertIsInstance(cursor, io.BufferedIOBase)

    def test_repr(self):
        buffer = self.make_buffer(b"abcd")
        cursor = Cursor(buffer, readonly=True)
        self.assertEqual(repr(cursor), "Cursor({!r}, readonly=True)".format(buffer))

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
        self.assertEqual(cursor.writable(), not memoryview(buf).readonly)
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
        self.assertTrue(cursor.readonly)
        self.assertRaises(io.UnsupportedOperation, cursor.write, b"blah\n")
        self.assertRaises(io.UnsupportedOperation, cursor.writelines, [b"blah\n"])

    def test_getvalue(self):
        buffer = self.make_buffer(b"abc")
        cursor = Cursor(buffer)
        self.assertIs(cursor.getvalue(), buffer)
        self.assertIsInstance(cursor.getvalue(), type(buffer))


class TestWriteCursorMixin:

    def test_write(self):
        buffer = self.make_buffer(bytearray(256))
        cursor = Cursor(buffer, readonly=False)

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
        # NOTE: `truncate` is not supported
        # self.assertEqual(cursor.truncate(12), 12)
        self.assertEqual(cursor.tell(), 1)

        value = cursor.getvalue()
        self.assertIs(value, buffer)
        self.assertEqual(value[:12], self.make_buffer(b"hello world\n"))


class TestCursorBytesMemoryview(unittest.TestCase, TestReadCursorMixin):

    @staticmethod
    def make_buffer(b):
        return memoryview(bytes(b))


class TestCursorBytes(unittest.TestCase, TestReadCursorMixin):

    @staticmethod
    def make_buffer(b):
        return bytes(b)


class TestCursorBytearray(unittest.TestCase, TestReadCursorMixin, TestWriteCursorMixin):

    @staticmethod
    def make_buffer(b):
        return bytearray(b)


class TestCursorArray(unittest.TestCase, TestReadCursorMixin, TestWriteCursorMixin):

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
