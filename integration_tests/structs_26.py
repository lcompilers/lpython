from lpython import dataclass, i32

@dataclass
class StringIO:
    """Imitate the parts of io.StringIO we need."""
    _buf     : str
    _0cursor : i32 = 0
    _len     : i32 = 0

def main0():
    io: StringIO = StringIO("random input xyz", 5, 24)

    print(io)
    assert io._buf == "random input xyz"
    assert io._0cursor == 5
    assert io._len == 24

main0()
