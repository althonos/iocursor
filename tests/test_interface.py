# coding: utf-8

import io
import unittest

from iocursor import Cursor


class TestBaseInterface(object):

    interface = None

    @classmethod
    def register(cls):
        for attr in dir(cls.interface):
            if not attr.startswith("_"):
                cls._register_test(attr)
        return cls

    @classmethod
    def _register_test(cls, name):

        def _test(self):
            nonlocal name
            self.assertTrue(hasattr(Cursor, name), f"Cursor class is missing {name!r} attribute")

        setattr(cls, f"test_hasattr_{name}", _test)


class TestIOBaseInterface(unittest.TestCase, TestBaseInterface):
    interface = io.IOBase


class TestBufferedIOBaseInterface(unittest.TestCase, TestBaseInterface):
    interface = io.BufferedIOBase


def load_tests(loader, suite, pattern):
    for cls in (TestIOBaseInterface, TestBufferedIOBaseInterface):
        tests = loader.loadTestsFromTestCase(cls.register())
        suite.addTests(tests)
    return suite
