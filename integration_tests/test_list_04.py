from math import sqrt
from ltypes import i32

def test_list_01():
    x: list[i32] = []
    y: list[i32] = []

    i: i32
    j: i32
    for i in range(5):
        j = (i ** 2) + 10
        x.insert(len(x), j)

    for i in x:
        j = int(sqrt(i - 10.0))
        y.append(j)

    for i in range(len(x)):
        x.remove((i ** 2) + 10)
        x.append(i)

    for i in range(len(y)):
        assert x[i] == y[i]

def test_list_02():
    x: list[str] = []

    i: i32
    for i in range(50):
        x.append(str(i + 1) + "_str")

    s: str
    for i in range(50):
        s = str(i + 1) + "_str"
        x.remove(s)

    assert 0 == len(x)

def tests():
    test_list_01()
    test_list_02()

tests()
