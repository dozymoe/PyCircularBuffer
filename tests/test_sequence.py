from circularbuffer import CircularBuffer

def test_sequence():
    buf = CircularBuffer(10)
    buf.write('12345')
    assert buf[0] == b'1'
    try:
        buf[6]
    except Exception as e:
        assert isinstance(e, IndexError)
    assert len(buf) == 5
    buf[3] = b'1'
    assert str(buf) == '12315'
    assert b'15' in buf
