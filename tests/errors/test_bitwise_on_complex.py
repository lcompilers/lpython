from ltypes import c32

def f():
    c1: c32
    c1 = 4+5j
    c2: c32
    c2 = -5.6-3j
    print(c1 | c2)


f()
