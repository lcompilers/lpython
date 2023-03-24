from lpython import i8, i16

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

def check():
    test_issue_1586()

check()
