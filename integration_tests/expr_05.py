from ltypes import i32, i64

def test_multiply(a: i32, b: i32) -> i32:
    return a*b

def test_mod(a: i32, b: i32) -> i32:
    return a%b

def main0():
    a: i32
    b: i32
    a = 10
    b = -5
    assert test_multiply(a, b) == -50
    i: i64
    i = 1
    i += int(1)
    assert i == 2
    a = 2
    b = 5
    assert test_mod(a, b) == 2
    assert test_mod(23, 3) == 2
    a = 123282374
    b = 32771
    assert test_mod(a, b) == 30643
    a = -5345
    b = -534
    assert a % b == -5
    a = -123282374
    b = 32771
    assert test_mod(a, b) == 2128

    assert 10 | 4 == 14
    assert -105346 | -32771 == -32769
    assert 10 & 4 == 0
    assert -105346 & -32771 == -105348
    assert 10 ^ 4 == 14
    assert -105346 ^ -32771 == 72579
    assert 10 >> 1 == 5
    assert 5 << 1 == 10
    i1: i32 = 10
    i2: i32 = 4
    assert i1 << i2 == 160
    assert i1 >> i2 == 0
    assert i1 & i2 == 0
    assert i1 | i2 == 14
    assert i1 ^ i2 == 14
    assert -i1 ^ -i2 == 10
    i3: i32 = 432534534
    i4: i32 = -4325
    assert i3 | i4 == -225
    assert i4 >> 3 == -541
    assert -i3 & i4 == -432534758
    assert -i3 ^ i4 == 432530657

main0()
