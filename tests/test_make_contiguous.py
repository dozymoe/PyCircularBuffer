from circularbuffer import CircularBuffer

def test_make_contiguous():
    buf = CircularBuffer(10)
    #'#          '#
    assert buf.write(b'1234') == 4
    #'1234#      '#
    buf.make_contiguous()
    assert str(buf) == '1234'

    assert buf.write(b'5678') == 4
    #'12345678#  '#
    buf.make_contiguous()
    assert str(buf) == '12345678'

    assert buf.read(2) == b'12'
    #'  345678#  '#
    assert buf.write(b'9012') == 4
    #'2#345678901'#
    assert str(buf) == '3456789012'
    buf.make_contiguous()
    #'3456789012#'#
    assert str(buf) == '3456789012'

    assert buf.read(7) == b'3456789'
    #'       012#'#
    assert str(buf) == '012'
    assert buf.write(b'1234567') == 7
    #'234567#0121'#
    assert str(buf) == '0121234567'
    buf.make_contiguous()
    #'0121234567#'#
    assert str(buf) == '0121234567'
