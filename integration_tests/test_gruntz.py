from lpython import S
from sympy import Symbol

def mmrv(e: S, x: S) -> list[S]:
    if not e.has(x):
        list0: list[S] = []
        return list0
    elif e == x:
        list1: list[S] = [x]
        return list1
    else:
        raise

def test_mrv():
    # Case 1
    x: S = Symbol("x")
    y: S = Symbol("y")
    ans1: list[S] = mmrv(y, x)
    print(ans1)
    assert len(ans1) == 0

    # Case 2
    ans2: list[S] = mmrv(x, x)
    ele1: S = ans2[0]
    print(ele1)
    assert ele1 == x
    assert len(ans2) == 1

test_mrv()