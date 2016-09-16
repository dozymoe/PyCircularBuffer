PyCircularBuffer
================

* License : MIT

Summary
-------

Simple `circular buffer <http://en.wikipedia.org/wiki/Circular_buffer>`_
written in python extension.


Installation
------------

Simply `pip install pycircularbuffer`, compilation from downloaded source
is probably needed.


Using
-----

 |   from circularbuffer import CircularBuffer
 |
 |   buf = CircularBuffer(1024)
 |
 |   buf.write(b'some text')
 |   while len(buf) > 0:
 |       buf.read(1)


API
---

Regular methods:
^^^^^^^^^^^^^^^^
* clear()
* read()
* resize()
* write()
* write_available()

String methods:
^^^^^^^^^^^^^^^
* count()
* startswith()

Sequence methods:
^^^^^^^^^^^^^^^^^
* __contains__()
* __getitem__()
* __len__()
* __setitem__()

Magic methods:
^^^^^^^^^^^^^^
* __repr__()
* __str__()

  Note: while string representation makes thing easier it always creates
  memory copy.
