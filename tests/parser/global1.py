x = "global "

def outer():
    x = "local"

    def inner():
        nonlocal x
        x = "nonlocal"
        assert x == "nonlocal"
    inner()

    assert x == "nonlocal"

def test_1():
    global x
    y = "local"
    x = x * 2
    assert x == "global global "
    assert y == "local"


def check():
    test_1()
    outer()

check()
