from setuptools import setup
from Cython.Build import cythonize

setup(
    ext_modules=cythonize(
        "IC.pyx",
        compiler_directives={'boundscheck': False, 'wraparound': False}
    )
)
