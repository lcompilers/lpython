from ltypes import c32

def f():
    c: c32
    c = c32(complex(3, 4))
    c.real = 5.0

f()
