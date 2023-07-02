from sympy import Symbol, expand, diff
from lpython import S

def test_operations():
    x: S = Symbol('x')
    y: S = Symbol('y')
    z: S = Symbol('z')
    
    # test expand
    a: S = (x + y)**S(2)
    b: S = (x + y + z)**S(3)
    assert(a.expand() == S(2)*x*y + x**S(2) + y**S(2))
    assert(expand(b) == S(3)*x*y**S(2) + S(3)*x*z**S(2) + S(3)*x**S(2)*y + S(3)*x**S(2)*z +\
    S(3)*y*z**S(2) + S(3)*y**S(2)*z + S(6)*x*y*z + x**S(3) + y**S(3) + z**S(3))
    print(a.expand())
    print(expand(b))

    # test diff
    c: S = (x + y)**S(2)
    d: S = (x + y + z)**S(3)
    assert(c.diff(x) == S(2)*(x + y))
    assert(diff(d, x) == S(3)*(x + y + z)**S(2))
    print(c.diff(x))
    print(diff(d, x))

test_operations()