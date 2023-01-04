def main0():
    x: i32
    x = 8
    s: str
    s = (7).to_bytes(5,'big')
    assert str(s) == r"b'\x00\x00\x00\x00\x07'"
    s = x.to_bytes(5,'big')
    assert str(s) == r"b'\x00\x00\x00\x00\x08'"

main0()
