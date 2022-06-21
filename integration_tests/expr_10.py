from ltypes import i32

def g() -> i32:
    return 5

def test_fn1():
    i: i32 = g()
    g()

test_fn1()
