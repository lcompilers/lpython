from sympy import Symbol, pi
from lpython import S

def test_symbolic_operations():
    x: S = Symbol('x')
    y: S = Symbol('y')
    p1: S = pi
    p2: S = pi
    
    # Addition
    z: S = x + y
    assert(z == x + y)
    print(z)
    
    # Subtraction
    w: S = x - y
    assert(w == x - y)
    print(w)
    
    # Multiplication
    u: S = x * y
    assert(u == x * y)
    print(u)

    # Division
    v: S = x / y
    assert(v == x / y)
    print(v)

    # Power
    p: S = x ** y
    assert(p == x ** y)
    print(p)

    # Casting
    a: S = S(100)
    b: S = S(-100)
    c: S = a + b
    assert(c == S(0))
    print(c)

    # Comparison
    b1: bool = p1 == p2
    print(b1)
    assert(b1 == True)
    b2: bool = p1 != pi
    print(b2)
    assert(b2 == False)
    b3: bool = p1 != x
    print(b3)
    assert(b3 == True)
    b4: bool = pi == Symbol("x")
    print(b4)
    assert(b4 == False)


test_symbolic_operations()
