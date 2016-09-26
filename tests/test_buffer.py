import re
import sys
from circularbuffer import CircularBuffer
from pytest import raises

def test_buffer():
    buf = CircularBuffer(15)
    regex = re.compile(br'(5)')

    # one segment
    assert buf.write(b'1234567890') == 10
    with buf as buf_:
        match = regex.search(buf_)
        assert match is not None

    # two segments
    assert buf.read(5) == b'12345'
    assert buf.write(b'1234567') == 7
    with buf:
        match = regex.search(buf)
        assert match is not None

    # python2 forced to use context manager
    if sys.version_info[0] < 3:
        with raises(TypeError):
            match = regex.search(buf)
    else:
        match = regex.search(buf)
