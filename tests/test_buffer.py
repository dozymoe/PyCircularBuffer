import re
from circularbuffer import CircularBuffer

def test_buffer():
    buf = CircularBuffer(1024)
    buf.write(b'ada apa dengan cinta')
    regex = re.compile(b'(n)')
