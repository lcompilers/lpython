from lpython import i64
from os import (open, read, close, write, O_RDONLY, O_WRONLY)

def test():
    path: str
    path = "../tmp.txt"
    fd: i64
    n: i64
    fd = open(path, O_WRONLY)
    b: str = b"Hello world!\n"
    write(fd, b)
    close(fd)

    fd = open(path, O_RDONLY)
    n = i64(100)
    s: str = read(fd, n)
    close(fd)
    
    print(s)
    assert b==s

test()
