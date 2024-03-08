from lpython import S
from sympy import Symbol, log

def func_01(e: S, x: S) -> S:
    print(e)
    if e == x:
        return x
    print(e)
    return e

def test_func_01():
    x: S = Symbol("x")
    ans: S = func_01(log(x), x)
    print(ans)

def func_02(e: S, x: S) -> list[S]:
    print(e)
    if e == x:
        list1: list[S] = [x]
        return list1
    else:
        print(e)
        list2: list[S] = func_02(x, x)
        return list2

def test_func_02():
    x: S = Symbol("x")
    ans: list[S] = func_02(log(x), x)
    ele: S = ans[0]
    print(ele)

def tests():
    test_func_01()
    test_func_02()

tests()