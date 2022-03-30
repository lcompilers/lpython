def f():
    x: str
    x = "ok"
    print(x)
    assert x == "ok"
    x = "abcdefghijkl"
    print(x)
    assert x == "abcdefghijkl"
    x = x + "123"
    print(x)
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

def check():
    f()
    test_str_concat()

check()
