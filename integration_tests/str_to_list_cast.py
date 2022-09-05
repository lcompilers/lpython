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

f()
