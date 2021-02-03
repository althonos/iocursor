#!/usr/bin/env python
# coding: utf-8

import os

import setuptools
from distutils.command.clean import clean as _clean
from setuptools.command.build_ext import build_ext as _build_ext
from setuptools.command.sdist import sdist as _sdist


# --- `setup.py` commands ----------------------------------------------------

class sdist(_sdist):
    """A `sdist` that generates a `pyproject.toml` on the fly.
    """

    def run(self):
        # build `pyproject.toml` from `setup.cfg`
        c = configparser.ConfigParser()
        c.add_section("build-system")
        c.set("build-system", "requires", str(self.distribution.setup_requires))
        c.set("build-system", 'build-backend', '"setuptools.build_meta"')
        with open("pyproject.toml", "w") as pyproject:
            c.write(pyproject)
        # run the rest of the packaging
        _sdist.run(self)


class build_ext(_build_ext):
    """A `build_ext` that disables optimizations if compiled in debug mode.
    """

    def info(self, message):
        self.announce(message, level=2)

    def warn(self, message):
        self.announce(message, level=3)

    def build_extension(self, ext):
        # update compile flags if compiling in debug mode
        if self.debug:
            if self.compiler.compiler_type in {"unix", "cygwin", "mingw32"}:
                self.info("adding C flags to compile without optimisations")
                ext.extra_compile_args.append("-O0")
                self.info("adding C flags to compile with coverage support")
                ext.extra_compile_args.append("-coverage")
            elif self.compiler.compiler_type == "msvc":
                self.info("adding C flags to compile without optimisations")
                ext.extra_compile_args.append("/Od")
            else:
                self.warn("unknown C compiler, cannot add debug flags")
        # run like normal
        _build_ext.build_extension(self, ext)


class clean(_clean):

    def run(self):

        source_dir = os.path.join(os.path.dirname(__file__), "pyhmmer")

        patterns = ["*.html"]
        if self.all:
            patterns.extend(["*.so", "*.c"])

        for pattern in patterns:
            for file in glob.glob(os.path.join(source_dir, pattern)):
                log.info("removing {!r}".format(file))
                os.remove(file)

        _clean.run(self)


# --- C extension ------------------------------------------------------------

extensions = [
    setuptools.Extension(
        "iocursor.cursor",
        [os.path.join("iocursor", "cursor.c")],
    )
]


# --- Setup ------------------------------------------------------------------

setuptools.setup(
    ext_modules=extensions,
    cmdclass=dict(
        build_ext=build_ext,
        clean=clean,
        sdist=sdist,
    )
)
