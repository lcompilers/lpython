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

f()
