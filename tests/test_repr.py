from circularbuffer import CircularBuffer

def test_repr():
    buf = CircularBuffer(70)
    buf.write(b'a' * 32)
    assert len(buf) == 32
    assert repr(buf) == '<CircularBuffer[32]:' + ('a' * 32) + '>'
    assert str(buf) == 'a' * 32
    buf.read(32)
    assert len(buf) == 0
    buf.write(b'b' * 64)
    assert len(buf) == 64
    assert repr(buf) == '<CircularBuffer[64]:' + ('b' * 41) + '..>'
    assert str(buf) == 'b' * 64
