from circularbuffer import CircularBuffer

BUFFER_SIZE = 256

def test_create():
    buf = CircularBuffer(BUFFER_SIZE)
    assert isinstance(buf, CircularBuffer)
    assert buf.write_available() == BUFFER_SIZE
    assert len(buf) == 0
    assert str(buf) == ''
    buf = CircularBuffer(size=BUFFER_SIZE)
    assert isinstance(buf, CircularBuffer)
    assert repr(buf) == '<CircularBuffer[0]:>'
