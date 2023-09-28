def main0():
    x: str
    x = "Hello, World"
    y: str
    y = "o"
    assert x.count(y) == 2
    y = ""
    assert x.count(y) == len(x) + 1
    y = "Hello,"
    assert x.count(y) == 1

main0()
