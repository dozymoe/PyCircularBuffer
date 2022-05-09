from circularbuffer import CircularBuffer

def test_resize():
    buf = CircularBuffer(10)
    assert buf.write_available() == 10
    buf.resize(5)
    assert buf.write_available() == 10
    buf.resize(15)
    assert buf.write_available() == 15
    buf.resize(size=20)
    assert buf.write_available() == 20
