def f():
    x: str
    x = "abcdefghijkl"
    x = x + "123"
    assert x == "abcdefghijkl123"
    g(x)
    y: str
    y = h(x)
    print(y)
    assert y == "abcdefghijkl123|x"

def g(x: str):
    print(x)
    assert x == "abcdefghijkl123"

def h(x: str) -> str:
    return x + "|x"


f()
