from lpython import i32

def test_bin():
    i: i32
    i = 5
    assert bin(i) == "0b101"
    i = 64
    assert bin(i) == "0b1000000"
    i = -534
    assert bin(i) == "-0b1000010110"
    assert bin(64) == "0b1000000"
    assert bin(-534) == "-0b1000010110"

test_bin()
