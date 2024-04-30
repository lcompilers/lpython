from lpython import S
from sympy import Symbol, log, E, Pow, exp

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
        base: S = e.args[0]
        exponent: S = e.args[1]
        one: S = S(1)
        if base != E:
            newe_exponent: S = S(1)
            newe: S = e
            while newe.func == Pow:
                newe_base: S = newe.args[0]
                newe_args1: S = newe.args[1]
                newe_exponent = newe_exponent * newe_args1
                newe = newe_base
            if newe_base == one:
                return empty_list
            if not newe_exponent.has(x):
                list3: list[S] = mmrv(newe_base, x)
                return list3
            else:
                # TODO as noted in #2526
                pass
        else:
            if exponent.func == log:
                list4: list[S] = mmrv(exponent.args[0], x)
                return list4
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

    # Case 5
    ans5: list[S] = mmrv(exp(log(x)), x)
    ele4: S = ans5[0]
    print(ele4)
    assert ele4 == x
    assert len(ans5) == 1

test_mrv()