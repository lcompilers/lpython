from ltypes import i8, i16, i32, i64

def test_new_line():
    print("abc\n")
    print("\ndef")
    print("x\nyz")

def test_int():
    i: i8 = i8(1)
    j: i16 = i16(2)
    k: i32 = 3
    l: i64 = i64(4)
    print("abc:", i, j, k, l)


def test_issue_1124():
    a: str
    a = "012345"
    assert a[-1] == "5"
    assert a[-1] == a[5]
    assert a[-2] == a[4]
    assert a[-4] == "2"


test_new_line()
test_int()
test_issue_1124()
