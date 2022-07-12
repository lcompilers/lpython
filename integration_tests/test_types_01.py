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

def test_i8_to_i64():
    i1: i8
    i1 = -5
    i2: i64
    i2 = i1
    assert i2 == -5

def test_i64_to_i8():
    i1: i64
    i1 = 6
    i2: i8
    i2 = i1
    assert i2 == 6

def check():
    test_i8()
    test_i16()
    test_i32()
    test_i64()
    test_i8_to_i64()
    test_i64_to_i8()


check()
