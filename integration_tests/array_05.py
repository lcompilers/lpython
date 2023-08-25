from lpython import i32, f64, Array
from numpy import empty, int32, float64


def test_1():
    y: Array[f64, 3] = empty([3], dtype=float64)
    y[0] = 3.14
    y[1] = -4.14
    y[2] = 100.100

    print(y)
    assert abs(y[0] - (3.14)) <= 1e-6
    assert abs(y[1] - (-4.14)) <= 1e-6
    assert abs(y[2] - (100.100)) <= 1e-6

def test_2():
    x: Array[i32, 2, 3] = empty([2, 3], dtype=int32)

    x[0, 0] = 5
    x[0, 1] = -10
    x[0, 2] = 15
    x[1, 0] = 4
    x[1, 1] = -14
    x[1, 2] = 100

    print(x)
    assert x[0, 0] == 5
    assert x[0, 1] == -10
    assert x[0, 2] == 15
    assert x[1, 0] == 4
    assert x[1, 1] == -14
    assert x[1, 2] == 100


def main0():
    test_1()
    test_2()

main0()
