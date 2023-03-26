from lpython import c32

def f():
    c1: c32
    c1 = c32(4)+c32(5j)
    c2: c32
    c2 = -c32(5.6)-c32(3j)
    print(c1 | c2)


f()
