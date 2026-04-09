from setuptools import setup

if __name__ == "__main__":
    setup(cffi_modules=["src/nexuschat/_build.py:ffibuilder"])
