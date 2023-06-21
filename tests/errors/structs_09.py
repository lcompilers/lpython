from lpython import dataclass, i32

@dataclass
class StringIO:
    _buf     : str = ''
    _len     : i32 = 5

def read() -> i32:
    return 5

def test_call_to_struct_member():
    fd : StringIO = StringIO('(Integer 4 [])', 5)
    bytes_read: i32 = fd.read()

test_call_to_struct_member()
