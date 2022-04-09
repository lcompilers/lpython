from ltypes import i32, i64

def test_multiply(a: i32, b: i32) -> i32:
    return a*b

def test_mod(a: i32, b: i32) -> i32:
    return a%b

def test_bitwise_or(a: i32, b: i32) -> i32:
    return a|b


def main0():
    a: i32
    b: i32
    a = 10
    b = -5
    assert test_multiply(a, b) == -50
    i: i64
    i = 1
    i += 1
    assert i == 2
    a = 2
    b = 5
    assert test_mod(a, b) == 2
    assert test_mod(23, 3) == 2
    a = 123282374
    b = 32771
    assert test_mod(a, b) == 30643
    a = -123282374
    b = 32771
    assert test_mod(a, b) == 2128

    a = 10
    b = 4
    assert test_bitwise_or(a, b) == 14
    a = -105346
    b = -32771
    assert test_bitwise_or(a, b) == -32769

main0()
