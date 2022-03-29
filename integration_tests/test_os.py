from ltypes import i64
from os import (open, read, close)

def test():
    path: str 
    path = "integration_tests/test_os.py"
    fd: i64
    n: i64
    fd = open(path, "r")
    n = 100
    print(read(fd, n))
    close(fd) 

test()
