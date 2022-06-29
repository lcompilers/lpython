from ltypes import pointer, i16, Pointer, i32

def f():
    yptr1: Pointer[i16[:]]
    y: i32[2]
    y[0] = 1
    y[1] = 2
    yptr1 = pointer(y)

f()
