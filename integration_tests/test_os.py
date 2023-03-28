from lpython import i64
from os import (open, read, close, write, O_RDONLY, O_WRONLY)

def test():
    path: str
    path = "../test_os.py"
    fd: i64
    n: i64
    fd = open(path, O_RDONLY)
    n = i64(100)
    print(read(fd, n))
    close(fd)

    path = "../tmp.txt"
    fd = open(path, O_WRONLY)
    write(fd, "Hello World!\n")
    close(fd)

test()
