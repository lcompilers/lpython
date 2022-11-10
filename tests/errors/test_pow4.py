from ltypes import c32, i64

def f():
    x: c32
    x = c32(2) + c32(3j)
    a: i64
    b: i64
    a = i64(2)
    b = i64(3)
    print(pow(x, a, b))
