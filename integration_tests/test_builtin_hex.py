from lpython import i32

def test_hex():
    i: i32
    i = 34
    assert hex(i) == "0x22"
    i = -4235
    assert hex(i) == "-0x108b"
    assert hex(34) == "0x22"
    assert hex(-4235) == "-0x108b"

test_hex()
