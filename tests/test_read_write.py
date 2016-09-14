from circularbuffer import CircularBuffer

def test_read_write():
    buf = CircularBuffer(15)
    assert buf.write_available() == 15

    written = buf.write(b'12345')
    assert written == 5
    assert buf.write_available() == 10
    assert str(buf) == '12345'

    assert buf.read(2) == b'12'
    assert buf.write_available() == 10
    assert str(buf) == '345'

    assert buf.read(size=3) == b'345'
    assert buf.write_available() == 10
    assert str(buf) == ''

    written = buf.write(b'123456789012345')
    assert written == 10
    assert buf.write_available() == 4
    assert str(buf) == '1234567890'

    written = buf.write(b'12')
    assert written == 2
    assert buf.write_available() == 2
    assert str(buf) == '123456789012'

    written = buf.write(b'123456')
    assert written == 2
    assert buf.write_available() == 0
    assert str(buf) == '12345678901212'

    buf.resize(20)
    assert buf.write_available() == 0
    assert str(buf) == '12345678901212'

    assert buf.read(10) == b'1234567890'
    assert buf.write_available() == 16
    assert str(buf) == '1212'
