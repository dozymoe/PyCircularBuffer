from circularbuffer import CircularBuffer

def test_read_write():
    buf = CircularBuffer(15)
    #'#               #'
    assert buf.write_available() == 15

    assert buf.write(b'12345') == 5
    #'12345#          #'
    assert buf.write_available() == 10
    assert str(buf) == '12345'

    assert buf.read(2) == b'12'
    #'  345#          #'
    assert buf.write_available() == 12
    assert str(buf) == '345'

    assert buf.read(size=3) == b'345'
    #'     #          #'
    assert buf.write_available() == 15
    assert str(buf) == ''

    assert buf.write(b'1234567890') == 10
    #'     1234567890##'
    assert buf.write_available() == 5
    assert str(buf) == '1234567890'

    assert buf.write(b'12') == 2
    #'2#   12345678901#'
    assert buf.write_available() == 3
    assert str(buf) == '123456789012'

    assert buf.write(b'123456') == 3
    #'2123#12345678901#'
    assert buf.write_available() == 0
    assert str(buf) == '123456789012123'

    assert buf.resize(20) == 20
    #'2123#12345678901     #'
    assert buf.write_available() == 0
    assert str(buf) == '123456789012123'

    assert buf.read(10) == b'1234567890'
    #'2123#          1     #'
    assert buf.write_available() == 10
    assert str(buf) == '12123'

    assert buf.read(2) == b'12'
    #' 123#                #'
    assert buf.write_available() == 17
    assert str(buf) == '123'
