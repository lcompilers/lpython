from lpython import pointer, i16, Pointer

# Testing Global Pointers
x: Pointer[i16[:]]

def f():
    yptr1: Pointer[i16[:]]
    y: i16[2]
    y[0] = i16(1)
    y[1] = i16(2)
    yptr1 = pointer(y)
    assert yptr1[0] == i16(1)
    assert yptr1[1] == i16(2)
    x = pointer(y)

def check():
    f()
    assert x[0] == i16(1)
    assert x[1] == i16(2)

check()
