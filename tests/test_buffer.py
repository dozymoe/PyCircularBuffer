import re
import sys
from circularbuffer import CircularBuffer
from pytest import raises

def test_buffer():
    buf = CircularBuffer(15)
    regex = re.compile(br'(5)')

    # one segment
    assert buf.write(b'12345') == 5
    with buf as buf_:
        match = regex.search(buf_)
        assert match is not None

    # floated one segment
    assert buf.read(5) == b'12345'
    assert buf.write(b'67890') == 5
    with buf as buf_:
        match = regex.search(buf_)
        assert match is None

    # two segments
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
