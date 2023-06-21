from sympy import Symbol
from lpython import S

def test_symbolic_operations():
    x: S = Symbol('x')
    y: S = Symbol('y')
    
    # Addition
    z: S = x + y
    print(z)  # Expected: x + y
    
    # Subtraction
    w: S = x - y
    print(w)  # Expected: x - y
    
    # Multiplication
    u: S = x * y
    print(u)  # Expected: x*y
    
    # Division
    v: S = x / y
    print(v)  # Expected: x/y
    
    # Power
    p: S = x ** y
    print(p)  # Expected: x**y
    
    # Casting
    a: S = S(100)
    b: S = S(-100)
    c: S = a + b
    print(c)  # Expected: 0

test_symbolic_operations()
