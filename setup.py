from distutils.core import setup, Extension

c_lib = Extension(
    'circularbuffer',
    sources=['src/circular_buffer.c'],
)

setup(
    name='pyCircularBuffer',
    version='1.0',
    description='Simple implementation of circular buffer.',
    author='Fireh',
    author_email='fireh@fireh.biz.id',
    ext_modules=[c_lib],
)
