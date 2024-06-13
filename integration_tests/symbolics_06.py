from sympy import Symbol, sin, cos, exp, log, Abs, pi, diff, sign
from lpython import S

def test_elementary_functions():

    # test sin, cos
    x: S = Symbol('x')
    assert(sin(pi) == S(0))
    assert(sin(pi/S(2)) == S(1))
    assert(sin(S(2)*pi) == S(0))
    assert(cos(pi) == S(-1))
    assert(cos(pi/S(2)) == S(0))
    assert(cos(S(2)*pi) == S(1))
    assert(diff(sin(x), x) == cos(x))
    assert(diff(cos(x), x) == S(-1)*sin(x))

    # test exp, log
    assert(exp(S(0)) == S(1))
    assert(log(S(1)) == S(0))
    assert(diff(exp(x), x) == exp(x))
    assert(diff(log(x), x) == S(1)/x)

    # test Abs
    assert(Abs(S(-10)) == S(10))
    assert(Abs(S(10)) == S(10))
    assert(Abs(S(-1)*x) == Abs(x))

    # test sign
    assert(sign(S(-10)) == S(-1))
    assert(sign(S(0)) == S(0))
    assert(sign(S(10)) == S(1))
    assert(sign(S(2)* x) == sign(x))
    assert(sign(S(-1)* x) == S(-1) * sign(x))

    # test composite functions
    a: S = exp(x)
    b: S = sin(a)
    b1: bool = b.func == sin
    c: S = cos(b)
    d: S = log(c)
    d1: bool = d.func == log
    e: S = Abs(d)
    print(e)
    assert(b1 == True)
    if b.func == sin:
        assert True
    else:
        assert False
    assert(b.func == sin)
    assert(d1 == True)
    if d.func == log:
        assert True
    else:
        assert False
    assert(d.func == log)
    assert(e == Abs(log(cos(sin(exp(x))))))

test_elementary_functions()