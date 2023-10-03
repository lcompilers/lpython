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

    # test is_Add, is_Mul & is_Pow
    a: S = x**pi
    b: S = x + pi
    c: S = x * pi
    assert(a.is_Pow == True)
    assert(b.is_Pow == False)
    assert(c.is_Pow == False)
    assert(a.is_Add == False)
    assert(b.is_Add == True)
    assert(c.is_Add == False)
    assert(a.is_Mul == False)
    assert(b.is_Mul == False)
    assert(c.is_Mul == True)


test_attributes()