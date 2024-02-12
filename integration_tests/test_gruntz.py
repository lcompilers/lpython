from lpython import S
from sympy import Symbol, log

def mmrv(e: S, x: S) -> list[S]:
    if not e.has(x):
        list0: list[S] = []
        return list0
    elif e == x:
        list1: list[S] = [x]
        return list1
    elif e.func == log:
        arg0: S = e.args[0]
        list2: list[S] = mmrv(arg0, x)
        return list2
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

    # Case 3
    ans3: list[S] = mmrv(log(x), x)
    ele2: S = ans3[0]
    print(ele2)
    assert ele2 == x
    assert len(ans2) == 1

test_mrv()