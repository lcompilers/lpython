from lpython import S
from sympy import Symbol

def mmrv(e: S, x: S) -> list[S]:
    l: list[S] = []
    if not e.has(x):
        return l
    else:
        raise

def test_mrv1():
    x: S = Symbol("x")
    y: S = Symbol("y")
    ans: list[S] = mmrv(y, x)
    assert len(ans) == 0

test_mrv1()