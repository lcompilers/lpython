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
    assert a[:4] == "0123"
    assert a[3:] == "345"
    # TODO:
    # assert a[0:5:-1] == ""


def test_str_istitle():
    a: str = "Hello World"
    b: str = "Hj'kl"
    c: str = "hELlo wOrlD"
    d: str = " Hello"
    e: str = " "
    res: bool = a.istitle()
    res2: bool = b.istitle()
    res3: bool = c.istitle()
    res4: bool = d.istitle()
    res5: bool = e.istitle()
    assert res == True
    assert res2 == False 
    assert res3 == False
    assert res4 == True
    assert res5 == False

def test_str_repeat():
    a: str
    a = "Xyz"
    assert a*3 == "XyzXyzXyz"
    assert a*2*3 == "XyzXyzXyzXyzXyzXyz"
    assert 3*a*3 == "XyzXyzXyzXyzXyzXyzXyzXyzXyz"
    assert a*-1 == ""

def test_str_join():
    a: str
    a = ","
    p:list[str] = ["a","b"]
    res:str = a.join(p)
    assert res == "a,b"

def test_str_join2():
    a: str
    a = "**"
    p:list[str] = ["a","b"]
    res:str = a.join(p)
    assert res == "a**b"

def test_str_join_empty_str():
    a: str
    a = ""
    p:list[str] = ["a","b"]
    res:str = a.join(p)
    assert res == "ab"

def test_str_join_empty_list():
    a: str
    a = "ab"
    p:list[str] = []
    res:str = a.join(p)
    assert res == ""

def test_constant_str_subscript():
    assert "abc"[2] == "c"
    assert "abc"[:2] == "ab"

def check():
    f()
    test_str_concat()
    test_str_index()
    test_str_slice()
    test_str_repeat()
    test_str_join()
    test_str_join2()
    test_str_join_empty_str()
    test_str_join_empty_list()
    test_constant_str_subscript()
    test_str_title()
    test_str_istitle()
    test_str_isalpha()

check()
