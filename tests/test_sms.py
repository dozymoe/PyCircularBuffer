from re import compile
from circularbuffer import CircularBuffer

def process_buffer(regex, buf, found):
    with buf:
        match = regex.match(buf)

    if found:
        assert match is not None
    else:
        assert match is None
        return

    match_str = match.group(0)
    match_str = buf.read(len(match_str))
    match = regex.match(match_str)
    if found:
        assert match is not None


def test_sms():
    buf = CircularBuffer(1024)
    regex_OK = compile(br'^\r\nOK\r\n')

    buf.write(b'\r\nOK\r\n')
    process_buffer(regex_OK, buf, True)

    buf.write(b'\r\nOK\r\n')
    process_buffer(regex_OK, buf, True)

    buf.write(b'\r\n+CGML:')
    process_buffer(regex_OK, buf, False)
