from ltypes import i32

def g(x: i32):
    print(x)

x: i32 = 7

def f():
    a: i32 = 5
    x: i32 = 3
    x = 5
    b: i32 = x + 1
    assert b == 6
    print(a, b)
    g(a*b + 3)

f()
