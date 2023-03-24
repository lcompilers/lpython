from lpython import c_p_pointer, CPtr, pointer, i16, Pointer, empty_c_void_p
from numpy import empty, int16

queries: CPtr = empty_c_void_p()
x: Pointer[i16[:]] = c_p_pointer(queries, i16[:])
print(queries, x)

def f():
    yq: CPtr = empty_c_void_p()
    yptr1: Pointer[i16[:]]
    y: i16[2] = empty(2, dtype=int16)
    y[0] = i16(1)
    y[1] = i16(2)
    yptr1 = pointer(y)
    print(pointer(y), yptr1)
    print(yptr1[0], yptr1[1])
    assert yptr1[0] == i16(1)
    assert yptr1[1] == i16(2)

    yptr1 = c_p_pointer(yq, i16[:])

    print(yq, yptr1)

f()
