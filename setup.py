from setuptools import setup, Extension, find_packages


c_lib = Extension(
    'circularbuffer',
    sources=[
        'src/circular_buffer.c',
        'src/base.c',
        'src/mapping.c',
        'src/methods.c',
        'src/sequence.c',
        'src/buffer.c',
    ],
    include_dirs=['src'],
)

setup(ext_modules=[c_lib])
