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

   
def test_str_title():
    a: str = "hello world"
    b: str = "hj'kl"
    c: str = "hELlo wOrlD"
    d: str = "{Hel1o}world"
    res: str = a.title()
    res2: str = b.title()
    res3: str = c.title()
    res4: str = d.title()
    assert res == "Hello World" 
    assert res2 == "Hj'Kl"
    assert res3 == "Hello World"
    assert res4 == "{Hel1O}World"

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

def test_str_split():
    a: str = "1,2,3"
    b: str = "1,2,,3,"
    c: str = "1and2and3"
    d: str = "1 2 3"
    e: str = "   1   2   3   "
    f: str = "123"
    res: list[str] = a.split(",")
    res1: list[str] = b.split(",")
    res2: list[str] = c.split("and")
    res3: list[str] = d.split()
    res4: list[str] = e.split()
    res5: list[str] = f.split(" ")
    # res6: list[str] = "".split(" ")
    assert res == ["1", "2", "3"]
    assert res1 == ["1", "2", "", "3", ""]
    assert res2 == ["1", "2", "3"]
    assert res3 == ["1", "2", "3"]
    assert res4 == ["1", "2", "3"]
    assert res5 == ["123"]
    # assert res6 == [""]

def test_str_replace():
    x: str = "abc"
    a: str = "zzaaabracadabra"
    print(a.replace("a",""))
    print(a.replace("",""))
    print(a.replace("a","b"))
    print(a.replace("e","a")) 
    print(a.replace("ab","ba"))
    print(a.replace("c","z"))
    print(a.replace("zza","yo"))
    print(a.replace("a","b",0))
    print(a.replace("a","b",1))
    print(a.replace("a","b",2))
    print(a.replace("a","b",2))
    print(a.replace("a","b",3))
    print(a.replace("a","b",4))
    print(a.replace("a","b",5))
    print(a.replace("a","b",6))
    print(a.replace("a","b",7))
    print(a.replace("a","b",8))
    print(a.replace("a","b",9))
    print(a.replace("b","k",1))
    print(a.replace("b","k",2))
    print(a.replace("zza","yo",2))
    print(x.replace("", ","))
    assert a.replace("a","") == "zzbrcdbr"
    assert a.replace("","") == "zzaaabracadabra"
    assert a.replace("a","b") == "zzbbbbrbcbdbbrb"
    assert a.replace("e","a") == "zzaaabracadabra"
    assert a.replace("ab","ba") == "zzaabaracadbara"
    assert a.replace("c","z") == "zzaaabrazadabra"
    assert a.replace("zza","yo") == "yoaabracadabra"
    assert a.replace("a","b",0) == "zzaaabracadabra"
    assert a.replace("a","b",1) == "zzbaabracadabra"
    assert a.replace("a","b",2) == "zzbbabracadabra"
    assert a.replace("a","b",2) == "zzbbabracadabra"
    assert a.replace("a","b",3) == "zzbbbbracadabra"
    assert a.replace("a","b",4) == "zzbbbbrbcadabra"
    assert a.replace("a","b",5) == "zzbbbbrbcbdabra"
    assert a.replace("a","b",6) == "zzbbbbrbcbdbbra"
    assert a.replace("a","b",7) == "zzbbbbrbcbdbbrb"
    assert a.replace("a","b",8) == "zzbbbbrbcbdbbrb"
    assert a.replace("a","b",9) == "zzbbbbrbcbdbbrb"
    assert a.replace("b","k",1) == "zzaaakracadabra"
    assert a.replace("b","k",2) == "zzaaakracadakra"
    assert a.replace("zza","yo",2) == "yoaabracadabra"
    assert x.replace("", ",") == ",a,b,c,"


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
    test_str_split()
    test_str_replace()

check()
