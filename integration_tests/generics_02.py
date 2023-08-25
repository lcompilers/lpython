from lpython import TypeVar, InOut, i32

T = TypeVar('T')

def swap(x: InOut[T], y: InOut[T]):
    temp: T
    temp = x
    x = y
    y = temp
    print(x)
    print(y)

def main0():
    a: i32 = 5
    b: i32 = 10

    # Invalid test case for CPython:
    # CPython passes values by copy of object reference
    # and hence does not support swapping variables
    # passed by arguments.
    # Therefore do not add asserts to this file

    print(a, b)

    swap(a, b)

    print(a, b)

main0()
