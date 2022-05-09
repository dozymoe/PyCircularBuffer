PyCircularBuffer
================

* License : MIT

Summary
-------

Simple `circular buffer <http://en.wikipedia.org/wiki/Circular_buffer>`_
written in python extension.

CircularBuffer will allocate requested size + 2 bytes for two sections of null
terminated strings.

May temporary allocate another half of the allocated bytes if you used buffer
protocol, I mostly used them for regex.


Installation
------------

Simply `pip install pycircularbuffer`, compilation from downloaded source
is probably needed.


Using
-----

.. code-block:: python

    from circularbuffer import CircularBuffer

    buf = CircularBuffer(1024)

    buf.write(b'some text')
    while len(buf) > 0:
        buf.read(1)

    from re import match

    buf.write(b'hallo')

    # python2
    with buf:
        match_found = match(br'^ha', buf)

    # python3
    match_found = match(br'^ha', buf)

    # use `result` immediately because regex didn't make memory copy of the
    # internal buffer, or run another `match()` on a memory copy, for example:

    match_found_str = buf.read(len(match_found.group(0)))
    independent_match_found = match(br'^ha', match_found_str)


Warning
-------

Don't to this in python 3: `'a' in buf`, instead: `b'a' in buf`.

Check the return value of write() which is the actual length that was written
into the buffer and also the exception that might be raised.


API
---

Regular methods:
^^^^^^^^^^^^^^^^
* clear()
* read()
* resize()
* write()
* write_available()
* make_contiguous()

String methods:
^^^^^^^^^^^^^^^
* count()
* startswith()
* find()
* index()

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

* __enter__()
* __exit__()

Buffer protocol:
^^^^^^^^^^^^^^^^
Note: for python version < 3, you need to use context manager, the `with`
statement, to let CircularBuffer know when you are releasing the buffer.

Using buffer protocol will throw `ReservedError` exception for
`CircularBuffer.read()`.
