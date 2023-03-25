from ltypes import i8, i16

def test_issue_1586():
    x4: i16
    y4: i16
    y4 = i16(10)
    x4 = i16(12345)
    assert x4//y4 == i16(1234)

    a4: i8
    b4: i8
    a4 = i8(10)
    b4 = i8(123)
    assert b4//a4 == i8(12)

def test_issue_1619():
    a: i16
    b: i16
    a = i16(10)
    b = i16(12345)
    assert b%a == i16(5)

    c: i8
    d: i8
    c = i8(10)
    d = i8(123)
    assert d%c == i8(3)

def check():
    test_issue_1586()
    test_issue_1619()

check()
