PyCircularBuffer
================

* License : MIT

Summary
-------

Simple [circular buffer][1] written in python extension.


Installation
------------

Simply `pip install pycircularbuffer`, compilation from downloaded source is
probably needed.


Using
-----

    from circularbuffer import CircularBuffer

    buf = CircularBuffer(1024)

    buf.write(b'some text')
    while len(buf) > 0:
        buf.read(1)



[1]: http://en.wikipedia.org/wiki/Circular_buffer
