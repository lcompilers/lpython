from sympy import Symbol, pi
from lpython import S

def test_operator_chaining():
    w: S = S(2)
    x: S = Symbol('x')
    y: S = Symbol('y')
    z: S = Symbol('z')
    Pi: S = Symbol('pi')

    a: S = x * w
    b: S = a + Pi
    c: S = b / z
    d: S = c ** w

    print(a)  # Expected: 2*x
    print(b)  # Expected: pi + 2*x
    print(c)  # Expected: (pi + 2*x)/z
    print(d)  # Expected: (pi + 2*x)**2/z**2

test_operator_chaining()