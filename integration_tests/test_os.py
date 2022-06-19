from ltypes import i64
from os import (open, read, close, O_RDONLY)

def test():
    path: str 
    path = "../test_os.py"
    fd: i64
    n: i64
    fd = open(path, O_RDONLY)
    n = 100
    print(read(fd, n))
    close(fd) 

test()
