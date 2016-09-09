from setuptools import setup, Extension, find_packages

c_lib = Extension(
    'circularbuffer',
    sources=['src/circular_buffer.c'],
)

setup(
    name='pycircularbuffer',
    version='0.0.1',
    url='https://github.com/dozymoe/PyCircularBuffer',
    download_url='https://github.com/dozymoe/PyCircularBuffer/tarball/0.0.1',
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
