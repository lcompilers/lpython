from ltypes import i32

def f():
    i: i32
    i = i32(int("314"))
    assert i == 314
    i = i32(int("-314"))
    assert i == -314
    s: str
    s = "123"
    i = i32(int(s))
    assert i == 123
    s = "-123"
    i = i32(int(s))
    assert i == -123
    s = "    1234"
    i = i32(int(s))
    assert i == 1234
    s = "    -1234   "
    i = i32(int(s))
    assert i == -1234

    assert int("  3   ") == 3
    assert int("+3") == 3
    assert int("\n3") == 3
    assert int("3\n") == 3
    assert int("\r\t\n3\r\t\n") == 3

f()
