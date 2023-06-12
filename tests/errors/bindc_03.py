
from lpython import c_p_pointer, CPtr, i32, Pointer, i16
from numpy import array

def fill_A(k: i32, n: i32, b: CPtr) -> None:
    A: Pointer[i16[:]] = c_p_pointer(b, i16[n * k], array([k * n]))
    i: i32; j: i32
    for j in range(k):
        for i in range(n):
            A[j*n+i] = i16((i+j))
