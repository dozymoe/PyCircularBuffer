from circularbuffer import CircularBuffer

def test_count():
    buf = CircularBuffer(32)
    buf.write(b'asdf\r\njkl;\r\n1234\r\n')
    assert buf.count(b'\r\n') == 3
    buf.clear()
    buf.write(b'asdf\r\njkl;\r\n1234\r\na')
    assert buf.count(b'\r\n') == 3
