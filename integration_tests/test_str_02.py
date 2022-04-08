def f():
    x: str
    x = "abcdefghijkl"
    x = x + "123"
    assert x == "abcdefghijkl123"
    g(x)
    y: str
    y = h(x)
    assert y == "abcdefghijkl123|x"

def g(x: str):
    assert x == "abcdefghijkl123"

def h(x: str) -> str:
    return x + "|x"


f()
