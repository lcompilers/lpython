from lpython import S
from sympy import Symbol, log, E, Pow

def mmrv(e: S, x: S) -> list[S]:
    empty_list : list[S] = []
    if not e.has(x):
        return empty_list
    elif e == x:
        list1: list[S] = [x]
        return list1
    elif e.func == log:
        arg0: S = e.args[0]
        list2: list[S] = mmrv(arg0, x)
        return list2
    elif e.func == Pow:
        if e.args[0] != E:
            e1: S = S(1)
            newe: S = e
            while newe.func == Pow:
                b1: S = newe.args[0]
                e1 = e1 * newe.args[1]
                newe = b1
            if b1 == S(1):
                return empty_list
            if not e1.has(x):
                list3: list[S] = mmrv(b1, x)
                return list3
            else:
                # TODO as noted in #2526
                pass
        else:
            # TODO
            pass
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
    assert len(ans3) == 1

    # Case 4
    ans4: list[S] = mmrv(x**S(2), x)
    ele3: S = ans4[0]
    print(ele3)
    assert ele3 == x
    assert len(ans4) == 1

test_mrv()