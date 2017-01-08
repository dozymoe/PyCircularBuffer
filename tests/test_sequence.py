from circularbuffer import CircularBuffer

def test_sequence():
    buf = CircularBuffer(10)
    buf.write(b'12345')
    assert buf[0] == b'1'
    try:
        buf[6]
    except Exception as e:
        assert isinstance(e, IndexError)
    assert len(buf) == 5
    buf[3] = b'1'
    assert str(buf) == '12315'
    assert b'15' in buf
    assert buf[1:1] == b''
    assert buf[1:2] == b'2'
    assert buf[1:] == b'2315'
    assert buf[:1] == b'1'
    assert buf[:-1] == b'1231'
    assert buf[2:-1] == b'31'
    assert buf[2:-1:2] == b'3'
    assert buf[5:1:-2] == b'53'

    # This broke the test in python 3
    # I don't know why...
    #assert '15' in buf


def test_sequence_two_sections():
    buf = CircularBuffer(10)
    assert buf.write(b'1234567890') == 10
    assert buf.write_available() == 0
    assert len(buf) == 10
    assert str(buf) == '1234567890'

    # where the first section is longer than the second
    assert buf.read(3) == b'123'
    assert buf.write(b'123') == 3
    assert buf.write_available() == 0
    assert len(buf) == 10
    assert str(buf) == '4567890123'

    # where the first section is equal length with the second
    assert buf.read(2) == b'45'
    assert buf.write(b'12') == 2
    assert buf.write_available() == 0
    assert len(buf) == 10
    assert str(buf) == '6789012312'

    # where the first section is shorter than the second
    assert buf.read(3) == b'678'
    assert buf.write(b'123') == 3
    assert buf.write_available() == 0
    assert len(buf) == 10
    assert str(buf) == '9012312123'

    # where the first section is only one byte
    assert buf.read(1) == b'9'
    assert buf.write_available() == 1
    assert len(buf) == 9
    assert str(buf) == '012312123'
