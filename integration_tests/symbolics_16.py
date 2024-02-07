from lpython import S
from sympy import Symbol, pi, sin

def mmrv() -> list[S]:
    x: S = Symbol('x')
    l1: list[S] = [pi, sin(x)]
    return l1

def test_mrv1():
    ans: list[S] = mmrv()
    element_1: S = ans[0]
    element_2: S = ans[1]
    assert element_1 == pi
    assert element_2 == sin(Symbol('x'))
    print(element_1, element_2)


test_mrv1()