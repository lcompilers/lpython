from ltypes import i32

def f():
    i: i32
    i = int("314")
    assert i == 314
    i = int("-314")
    assert i == -314
    s: str
    s = "123"
    i = int(s)
    assert i == 123
    s = "-123"
    i = int(s)
    assert i == -123
    s = "    1234"
    i = int(s)
    assert i == 1234
    s = "    -1234   "
    i = int(s)
    assert i == -1234

f()
