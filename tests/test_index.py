from circularbuffer import CircularBuffer
from pytest import raises

def test_index():
    buf = CircularBuffer(32)
    buf.write(b'asdf\r\njkl;\r\n1234\r\n')
    assert buf.index(b'\r\n') == 4
    assert buf.index(b'\r\n', 5) == 10
    with raises(ValueError):
        buf.index(b'x')

    buf.clear()
    buf.write(b'asdf\r\njkl;\r\n1234\r\na')
    assert buf.index(b'\r\n') == 4
    assert buf.index(b'\r\n', 5) == 10
    with raises(ValueError):
        buf.index(b'x')

    with raises(ValueError):
        buf.index(b'')
