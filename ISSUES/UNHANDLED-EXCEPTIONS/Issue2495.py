from lpython import i16, Allocatable
from numpy import empty, int16


def foo() -> Allocatable[i16[:]]:
    result: i16[1] = empty((1,), dtype=int16)
    return result