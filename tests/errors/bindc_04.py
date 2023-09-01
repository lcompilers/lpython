from lpython import CPtr, i32, Pointer, i16
from numpy import empty, int16

def fill_A(k: i32, n: i32) -> None:
    A: i16[n*k] = empty(n*k, dtype=int16)
    i: i32; j: i32
    for j in range(k):
        for i in range(n):
            A[j*n+i] = i16((i+j))

def fill_B(k: i32, n: i32) -> None:
    B: i16[k*n] = empty(k*n, dtype=int16)
    i: i32; j: i32
    for j in range(k):
        for i in range(n):
            B[j*n+i] = i16((i+j))

def fill_C(k: i32, n: i32, b: CPtr) -> None:
    nk: i32 = n * k
    C: i16[nk] = empty(nk, dtype=int16)
    i: i32; j: i32
    for j in range(k):
        for i in range(n):
            C[j*n+i] = i16((i+j))
