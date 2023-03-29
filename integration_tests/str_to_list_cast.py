from lpython import i32

def f():
    x: list[str]
    x = list("lpython")
    s: str
    s = "lpython"
    i: i32
    for i in range(len(s)):
        assert x[i] == s[i]
    x = list("")
    assert len(x) == 0
    x = list("L")
    assert len(x) == 1 and x[0] == 'L'
    x = list(s)
    assert len(x) == len(s)
    for i in range(len(x)):
        assert x[i] == s[i]
    s = "agowietg348203wk.smg.afejwp398273wd.a,23to0MEG.F,"
    x = list(s)
    assert len(x) == len(s)
    for i in range(len(x)):
        assert x[i] == s[i]
        s += str(i)
    x = list(s)
    assert len(x) == len(s)
    for i in range(len(x)):
        assert x[i] == s[i]


f()
