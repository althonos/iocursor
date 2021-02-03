# coding: utf-8

import io
import unittest

from iocursor import Cursor


class TestReadCursorMixin:

    def test_interface(self):
        cursor = Cursor(self.buftype(b"abcd"))
        self.assertIsInstance(cursor, io.IOBase)
        self.assertIsInstance(cursor, io.BufferedIOBase)

    def test_repr(self):
        buffer = self.buftype(b"abcd")
        cursor = Cursor(buffer, mode="rw")
        self.assertEqual(repr(cursor), "Cursor({!r}, mode='rw')".format(buffer))

        class custom_repr(self.buftype):
            def __repr__(self):
                return "<custom_repr>"

        buffer_custom = custom_repr(b"")
        cursor = Cursor(buffer_custom)
        self.assertEqual(repr(cursor), "Cursor(<custom_repr>, mode='r')")

    def test_read(self):
        cursor = Cursor(self.buftype(b"abcdefghijkl"))
        self.assertEqual(cursor.read(2), b"ab")
        self.assertEqual(cursor.read(3), b"cde")
        self.assertEqual(cursor.read(0), b"")
        self.assertEqual(cursor.read(), b"fghijkl")
        self.assertEqual(cursor.read(2), b"")
        self.assertEqual(cursor.read(), b"")

    def test_close(self):
        cursor = Cursor(self.buftype(b"abcd"))
        self.assertFalse(cursor.closed)
        cursor.close()
        self.assertTrue(cursor.closed)
        cursor.close()
        self.assertTrue(cursor.closed)

    def test_mode(self):
        cursor = Cursor(self.buftype(b""))
        self.assertEqual(cursor.mode, "r")
        cursor = Cursor(self.buftype(b""), mode="rb")
        self.assertEqual(cursor.mode, "rb")

    def test_tell(self):
        cursor = Cursor(self.buftype(b"abcdefghijkl"))
        self.assertEqual(cursor.tell(), 0)
        cursor.read(4)
        self.assertEqual(cursor.tell(), 4)
        cursor.read()
        self.assertEqual(cursor.tell(), 12)


class TestCursorBytes(unittest.TestCase, TestReadCursorMixin):
    buftype = bytes


class TestCursorBytearray(unittest.TestCase, TestReadCursorMixin):
    buftype = bytearray
