def f():
    x: str
    x = "ok"
    assert x == "ok"
    x = "abcdefghijkl"
    assert x == "abcdefghijkl"
    x = x + "123"
    assert x == "abcdefghijkl123"

def test_str_concat():
    a: str
    a = "abc"
    b: str
    b = "def"
    c: str
    c = a + b
    assert c == "abcdef"
    a = ""
    b = "z"
    c = a + b
    assert c == "z"

def test_str_index():
    a: str
    a = "012345"
    assert a[2] == "2"
    assert a[-1] == "5"
    assert a[-6] == "0"

def test_str_slice():
    a: str
    a = "012345"
    assert a[2:4] == "23"
    assert a[2:3] == a[2]

def test_str_repeat():
    a: str
    a = "Xyz"
    assert a*3 == "XyzXyzXyz"
    assert a*2*3 == "XyzXyzXyzXyzXyz"
    assert 3*a*3 == "XyzXyzXyzXyzXyzXyzXyzXyzXyz"
    assert a*-1 == ""

def check():
    f()
    test_str_concat()
    test_str_index()
    test_str_slice()
    test_str_repeat()

check()
