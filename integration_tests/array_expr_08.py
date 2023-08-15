from lpython import i1
from numpy import bool_, array

def g():
    a1: i1[4] = array([0, -127, 0, 111], dtype=bool_)

    print(a1)

    assert not a1[0]
    assert a1[1]
    assert not a1[2]
    assert a1[3]

g()
