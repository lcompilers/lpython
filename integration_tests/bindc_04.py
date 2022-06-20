from ltypes import pointer, i16, Pointer

# Testing Global Pointers
x: Pointer[i16[:]]

def f():
    yptr1: Pointer[i16[:]]
    y: i16[2]
    y[0] = 1
    y[1] = 2
    yptr1 = pointer(y)
    assert yptr1[0] == 1
    assert yptr1[1] == 2
    x = pointer(y)

def check():
    f()
    assert x[0] == 1
    assert x[1] == 2

check()
