from ltypes import pointer, i16, Pointer

def g(x: Pointer[i16[:]], y: i16[:]):
    y[0] = 1
    y[1] = 2
    x = pointer(y)
    print(x[0], x[1])

def check(ptr: Pointer[i16[:]]):
    assert ptr[0] == 1
    assert ptr[1] == 2

def f():
    yptr1: Pointer[i16[:]]
    y: i16[2]
    g(yptr1, y)
    check(yptr1)

f()
