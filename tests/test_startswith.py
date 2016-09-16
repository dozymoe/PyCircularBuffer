from circularbuffer import CircularBuffer

def test_startswith():
    buf = CircularBuffer(1024)
    buf.write(b'1234567890')
    assert buf.startswith(b'123') == 1
    assert buf.startswith(b'678') == 0
    assert buf.startswith(b'1234567890') == 1
