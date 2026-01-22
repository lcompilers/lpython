from numpy import empty, int16

from lpython import (i16, i32, Allocatable)


# doesn't work:
# def to_lpython_array(n: i32, m: i32) -> Array[i16, n, m]: #ndarray(Any, dtype=int16):
# works:
# def to_lpython_array(n: i32, m: i32) -> Array[i16, 15, 3]: #ndarray(Any, dtype=int16):
# doesn't work:
def to_lpython_array(n: i32, m: i32) -> Allocatable[i16[:]]:
    A_nm: i16[n, m] = empty((n, m), dtype=int16)
    return A_nm


