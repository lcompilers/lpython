from sympy import Symbol, expand, diff, sin, cos, exp, pi
from lpython import S

def test_operations():
    x: S = Symbol('x')
    y: S = Symbol('y')
    z: S = Symbol('z')
    a: S = (x + y)**S(2)
    b: S = (x + y + z)**S(3)

    # test expand
    assert(a.expand() == S(2)*x*y + x**S(2) + y**S(2))
    assert(expand(b) == S(3)*x*y**S(2) + S(3)*x*z**S(2) + S(3)*x**S(2)*y + S(3)*x**S(2)*z +\
    S(3)*y*z**S(2) + S(3)*y**S(2)*z + S(6)*x*y*z + x**S(3) + y**S(3) + z**S(3))
    print(a.expand())
    print(expand(b))

    # test diff
    assert(a.diff(x) == S(2)*(x + y))
    assert(diff(b, x) == S(3)*(x + y + z)**S(2))
    print(a.diff(x))
    print(diff(b, x))

    # test diff 2
    c:S = sin(x)
    d:S = cos(x)
    assert(sin(Symbol("x")).diff(x) == d)
    assert(sin(x).diff(Symbol("x")) == d)
    assert(sin(x).diff(x) == d)
    assert(sin(x).diff(x).diff(x) == S(-1)*c)
    assert(sin(x).expand().diff(x).diff(x) == S(-1)*c)
    assert((sin(x) + cos(x)).diff(x) == S(-1)*c + d)
    assert((sin(x) + cos(x) + exp(x) + pi).diff(x).expand().diff(x) == exp(x) + S(-1)*c + S(-1)*d)


test_operations()