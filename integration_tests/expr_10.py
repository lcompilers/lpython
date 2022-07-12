from ltypes import i32

def g() -> i32:
    return 5

def gsubrout(x: i32):
    print(x)

def test_fn1():
    i: i32 = g()
    j: i32
    j = g()
    g()
    gsubrout(i)

test_fn1()
