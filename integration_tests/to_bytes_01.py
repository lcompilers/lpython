def main0():
    x: i32
    x = 100
    s: str
    s = (7).to_bytes(5,'big')
    print(s)
    assert str(s) == r"b'\x00\x00\x00\x00\x07'"
    #s = x.to_bytes(3,'big')
    #print(s)
    #assert s == "(TODO: 100)"

main0()
