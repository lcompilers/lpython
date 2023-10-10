from sympy import Symbol, sin, pi
from lpython import S

def test_extraction_of_elements():
    x: S = Symbol("x")
    l1: list[S] = [x, pi, sin(x), Symbol("y")]
    ele1: S = l1[0]
    ele2: S = l1[1]
    ele3: S = l1[2]
    ele4: S = l1[3]

    assert(ele1 == x)
    assert(ele2 == pi)
    assert(ele3 == sin(x))
    assert(ele4 == Symbol("y"))
    print(ele1, ele2, ele3, ele4)

test_extraction_of_elements()