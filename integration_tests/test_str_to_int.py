from lpython import i32

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

    assert i32(int("01010",10)) == 1010
    assert i32(int("01010",2)) == 10
    assert i32(int("Beef",16)) == 48879
    assert i32(int("0xE",16)) == 14
    assert i32(int("0xE",0)) == 14
    assert i32(int("123",0)) == 123
    assert i32(int("0bE",16)) == 190

f()
