from sympy import Symbol, pi, S
from lpython import S

def test_chained_operations():
    x: S = Symbol('x')
    y: S = Symbol('y')
    z: S = Symbol('z')
    a: S = Symbol('a')
    b: S = Symbol('b')
    
    # Chained Operations
    w: S = (x + y) * ((a - b) / (pi + z))
    result: S = (w ** S(2) - pi) + S(3)
    
    # Print Statements
    assert(result == S(3) + (a -b)**S(2)*(x + y)**S(2)/(z + pi)**S(2) - pi)
    print(result)
    # Expected: 3 + (a - b)**2*(x + y)**2/(z + pi)**2 - pi
    
    # Additional Variables
    c: S = Symbol('c')
    d: S = Symbol('d')
    e: S = Symbol('e')
    f: S = Symbol('f')
    
    # Chained Operations with Additional Variables
    x = (c * d + e) / f
    y = (x - S(10)) * (pi + S(5))
    z = y ** (S(2) / (f + d))
    result = (z + e) * (a - b)
    
    # Print Statements
    assert(result == (a - b)*(e + ((S(5) + pi)*(S(-10) + (e + c*d)/f))**(S(2)/(d + f))))
    print(result)
    # Expected: (a - b)*(e + ((5 + pi)*(-10 + (e + c*d)/f))**(2/(d + f)))
    assert(x == (e + c*d)/f)
    assert(y == (S(5) + pi)*(S(-10) + (e + c*d)/f))
    assert(z == ((S(5) + pi)*(S(-10) + (e + c*d)/f))**(S(2)/(d + f)))

test_chained_operations()