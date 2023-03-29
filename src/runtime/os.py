from lpython import i32, i64, ccall

O_RDONLY: i32 # = 0 FIXME: Assign the value 0 to O_RDONLY
# O_WRONLY: i32 = 1
# O_RDWR  : i32 = 2
# O_CREAT : i32 = 64
# O_APPEND: i32 = 1024


def open(path: str, flag: i32) -> i64:
    """
    Returns the file descriptor for the newly opened file
    """
    sflag: str
    if flag == O_RDONLY:
        sflag = "r"
    else:
        quit(1) # not implemented yet
    return _lpython_open(path, sflag)

@ccall
def _lpython_open(path: str, flag: str) -> i64:
    pass

def read(fd: i64, n: i64) -> str:
    """
    Reads at most `n` bytes from file descriptor
    """
    return _lpython_read(fd, n)

@ccall
def _lpython_read(fd: i64, n: i64) -> str:
    pass

def close(fd: i64):
    """
    Closes the file descriptor
    """
    _lpython_close(fd)
    return

@ccall
def _lpython_close(fd: i64):
    pass
