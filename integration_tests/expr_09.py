from ltypes import i32

def main0():
    i1: i32 = 10
    i2: i32 = 4
    i1 = 3
    i2 = 5
    assert -i1 ^ -i2 == 6

main0()
