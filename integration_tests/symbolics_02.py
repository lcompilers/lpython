from sympy import Symbol
from lpython import S

def test_symbolic_operations():
    x: S = Symbol('x')
    y: S = Symbol('y')
    
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

test_symbolic_operations()
