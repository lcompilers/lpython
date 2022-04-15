from ltypes import i8, i16, i32, i64

def test_i8():
    i: i8
    i = 5
    assert i == 5

def test_i16():
    i: i16
    i = 5
    assert i == 5

def test_i32():
    i: i32
    i = 5
    assert i == 5

def test_i64():
    i: i64
    i = 5
    assert i == 5


def check():
    test_i8()
    test_i16()
    test_i32()
    test_i64()


check()
