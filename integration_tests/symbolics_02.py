from sympy import Symbol, pi, Add, Mul, Pow
from lpython import S

def test_symbolic_operations():
    x: S = Symbol('x')
    y: S = Symbol('y')
    pi1: S = pi
    pi2: S = pi
    
    # Addition
    z: S = x + y
    z1: bool = z.func == Add
    z2: bool = z.func == Mul
    assert(z == x + y)
    assert(z1 == True)
    assert(z2 == False)
    if z.func == Add:
        assert True
    else:
        assert False
    assert(z.func == Add)
    print(z)
    
    # Subtraction
    w: S = x - y
    w1: bool = w.func == Add
    assert(w == x - y)
    assert(w1 == True)
    if w.func == Add:
        assert True
    else:
        assert False
    assert(w.func == Add)
    print(w)
    
    # Multiplication
    u: S = x * y
    u1: bool = u.func == Mul
    assert(u == x * y)
    assert(u1 == True)
    if u.func == Mul:
        assert True
    else:
        assert False
    assert(u.func == Mul)
    print(u)

    # Division
    v: S = x / y
    v1: bool = v.func == Mul
    assert(v == x / y)
    assert(v1 == True)
    if v.func == Mul:
        assert True
    else:
        assert False
    assert(v.func == Mul)
    print(v)

    # Power
    p: S = x ** y
    p1: bool = p.func == Pow
    p2: bool = p.func == Add
    p3: bool = p.func == Mul
    assert(p == x ** y)
    assert(p1 == True)
    assert(p2 == False)
    assert(p3 == False)
    if p.func == Pow:
        assert True
    else:
        assert False
    assert(p.func == Pow)
    print(p)

    # Casting
    a: S = S(100)
    b: S = S(-100)
    c: S = a + b
    assert(c == S(0))
    print(c)

    # Comparison
    b1: bool = pi1 == pi2
    print(b1)
    assert(b1 == True)
    b2: bool = pi1 != pi
    print(b2)
    assert(b2 == False)
    b3: bool = pi1 != x
    print(b3)
    assert(b3 == True)
    b4: bool = pi == Symbol("x")
    print(b4)
    assert(b4 == False)


test_symbolic_operations()
