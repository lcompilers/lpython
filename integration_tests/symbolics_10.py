from sympy import Symbol, sin, pi
from lpython import S

def test_attributes():
    w: S = pi
    x: S = Symbol('x')
    y: S = Symbol('y')
    z: S = sin(x)

    # test has
    assert(w.has(x) == False)
    assert(y.has(x) == False)
    assert(x.has(x) == True)
    assert(x.has(x) == z.has(x))

    # test has 2
    assert(sin(x).has(x) == True)
    assert(sin(x).has(y) == False)
    assert(sin(Symbol("x")).has(x) == True)
    assert(sin(Symbol("x")).has(y) == False)
    assert(sin(Symbol("x")).has(Symbol("x")) == True)
    assert(sin(Symbol("x")).has(Symbol("y")) == False)
    assert(sin(Symbol("x")).has(Symbol("x")) != sin(Symbol("x")).has(Symbol("y")))
    assert(sin(Symbol("x")).has(Symbol("x")) == sin(Symbol("y")).has(Symbol("y")))

test_attributes()