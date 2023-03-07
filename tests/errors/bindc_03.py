
from ltypes import c_p_pointer, CPtr, i32, Pointer, i16

def fill_A(k: i32, n: i32, b: CPtr) -> None:
    nk: i32 = n * k
    A: Pointer[i16[nk]] = c_p_pointer(b, i16[k * n])
    i: i32; j: i32
    for j in range(k):
        for i in range(n):
            A[j*n+i] = i16((i+j))

def fill_B(k: i32, n: i32, b: CPtr) -> None:
    B: Pointer[i16[n * k]] = c_p_pointer(b, i16[k * n])
    i: i32; j: i32
    for j in range(k):
        for i in range(n):
            B[j*n+i] = i16((i+j))

def fill_C(k: i32, n: i32, b: CPtr) -> None:
    C: Pointer[i16[n]] = c_p_pointer(b, i16[n * k])
    i: i32; j: i32
    for j in range(k):
        for i in range(n):
            C[j*n+i] = i16((i+j))
