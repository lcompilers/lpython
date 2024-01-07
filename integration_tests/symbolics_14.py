from lpython import S
from sympy import Symbol

def mmrv(x: S) -> list[S]:
    l1: list[S] = [x]
    return l1

def test_mrv1():
    x: S = Symbol("x")
    ans: list[S] = mmrv(x)
    element_1: S = ans[0]
    print(element_1)
    assert element_1 == x

test_mrv1()