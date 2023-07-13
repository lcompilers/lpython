from lpython import i32, Const
from numpy import empty, int32

def main0():
    n: Const[i32] = 1
    x: i32[n, n] = empty([n, n], dtype=int32)
    y: i32[n, n] = empty([n, n], dtype=int32)

    x[0, 0] = -10
    y[0, 0] = -10

    print(x[0, 0], y[0, 0])
    assert x == y

    y[0, 0] = 10
    print(x[0, 0], y[0, 0])
    assert x != y

main0()
