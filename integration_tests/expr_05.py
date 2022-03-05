from ltypes import i32

def test_multiply(a: i32, b: i32) -> i32:
    return a*b

def main0():
    a: i32
    b: i32
    a = 10
    b = -5
    assert test_multiply(a, b) == -50

main0()
