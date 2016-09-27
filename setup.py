from setuptools import setup, Extension, find_packages

RELEASE_VERSION = '0.0.6'

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

setup(
    name='pycircularbuffer',
    version=RELEASE_VERSION,
    url='https://github.com/dozymoe/PyCircularBuffer',
    download_url='https://github.com/dozymoe/PyCircularBuffer/tarball/' +\
            RELEASE_VERSION,

    author='Fahri Reza',
    author_email='dozymoe@gmail.com',
    description='Simple implementation of circular buffer.',
    packages=find_packages(exclude=['tests']),
    zip_safe=False,
    include_package_data=True,
    platforms='any',
    license='MIT',
    install_requires=[],
    ext_modules=[c_lib],
)
