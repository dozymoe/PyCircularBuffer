from circularbuffer import CircularBuffer
from pytest import raises

def test_find():
    buf = CircularBuffer(32)
    buf.write(b'asdf\r\njkl;\r\n1234\r\n')
    assert buf.find(b'\r\n') == 4
    assert buf.find(b'x') == -1
    assert buf.find(b'\r\n', 5) == 10

    buf.clear()
    buf.write(b'asdf\r\njkl;\r\n1234\r\na')
    assert buf.find(b'\r\n') == 4
    assert buf.find(b'x') == -1
    assert buf.find(b'\r\n', 5) == 10

    with raises(ValueError):
        buf.find(b'')
