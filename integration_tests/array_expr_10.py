from lpython import i32
from numpy import empty, int32, array

def foo(x: i32[:]):
    print(x[3], x[4], x[-1], x[-2])
    assert x[-1] == 5
    assert x[-2] == 4
    assert x[-3] == 3
    assert x[-4] == 2
    assert x[-5] == 1

def main():
    x: i32[5] = empty(5, dtype=int32)
    x = array([1, 2, 3, 4, 5])
    foo(x)

main()