from numpy import array, empty, int16
from lpython import (i16, i32, c_p_pointer, Pointer, CPtr, TypeVar)


Tn = TypeVar("Tn")
Tm = TypeVar("Tm")
Tl = TypeVar("Tl")


def THIS_WORKS(Anm_l4: CPtr, Tn: i32, Tm: i32, l: i32) -> i16[Tn, Tm]:
    A_nm: i16[Tn, Tm] = empty((Tn, Tm), dtype=int16)
    return A_nm


def THIS_DOESNT_WORK(d: i16[Tm, Tn], b: CPtr, Tm: i32, Tn: i32) -> None:
    B: Pointer[i16[:]] = c_p_pointer(b, i16[:], array([Tm * Tn]))
    i: i32
    j: i32
    for i in range(Tm):
        for j in range(Tn):
            d[i, j] = B[(i * Tn) + j]

