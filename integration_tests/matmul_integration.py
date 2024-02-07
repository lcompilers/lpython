import numpy
from numpy import array, empty, int16
from lpython import (i16, i32, ccallback, c_p_pointer, Pointer, u64, CPtr, i64,
                     ccall, sizeof, Array, Allocatable, TypeVar, Const)

######## ALL THE LINES WITH EIGHT COMMENT MARKS ARE THE ONES WE NEED TO
######## BRING UP!  AS IT STANDS, THIS CODE WORKS IN LPYTHON MAIN AS OF 4
######## FEBRUARY 2024.

# https://numpy.org/devdocs/reference/typing.html
######## from numpy.typing import NDArray


# plan for 30 Jan 2024 --
# step 0: comment out this code and ./build_baryon.sh to run on APU
#         emulator; or ./run_full_emulation.sh to run in CPython.
# step 1: side-by-side numpy implementation in full-emulation
#         - get there line-by-line
#         = focus on gvml_add_u16 first


@ccall
def _lfortran_malloc(size : i32) -> CPtr:
    """Borrow from bindc_07.py in integration_tests."""
    pass


def init_c_fortran_array(b: CPtr, rows: i32, cols: i32, mod: i32) -> None:
    """Initialize a C / Fortran array with test data."""
    B: Pointer[i16[:]] = c_p_pointer(b, i16[:], array([rows * cols]))
    i: i32
    j: i32
    for i in range(rows):
        for j in range(cols):
            B[(i * cols) + j] = i16((i + j) % mod)


def zero_c_fortran_array(b: CPtr, rows: i32, cols: i32) -> None:
    """Zero out a C / Fortran array."""
    B: Pointer[i16[:]] = c_p_pointer(b, i16[:], array([rows * cols]))
    i: i32
    j: i32
    for i in range(rows):
        for j in range(cols):
            B[(i * cols) + j] = i16(0)


rows = TypeVar("rows")
cols = TypeVar("cols")


def load_lpython_array_from_c_fortran_array(b: CPtr, rows: i32, cols: i32) -> i16[rows, cols]:
    """Load an LPython array from a C / Fortran array."""
    B: Pointer[i16[:]] = c_p_pointer(b, i16[:], array([rows * cols]))
    D: i16[rows, cols] = empty((rows, cols), dtype=int16)
    i: i32
    j: i32
    for i in range(rows):
        for j in range(cols):
            D[i, j] = B[(i * cols) + j]
    return D


def spot_print(a: i16[:]) -> None:
    """Issue2497"""
    return


def clear_row(a: i16[:], row: i32, cols: i32) -> None:
    j: i32
    for j in range(cols):
        a[row, j] = i16(0)


def main() -> i32:

    # "Const" lets these appear in type declarations such as i16[n, m]
    n  : Const[i32] = 15
    m  : Const[i32] = 3
    l  : Const[i32] = 32_768

    M1 : i32 = 1
    M2 : i32 = 5  # Issue 2499 -- can't be Const

    Anm_l4 : CPtr = _lfortran_malloc((n * m) * i32(sizeof(i16)))
    Bml_l4 : CPtr = _lfortran_malloc((m * l) * i32(sizeof(i16)))
    Cnl_l4 : CPtr = _lfortran_malloc((n * l) * i32(sizeof(i16)))

    init_c_fortran_array(Anm_l4, n, m, 11)
    init_c_fortran_array(Bml_l4, m, l, 13)
    zero_c_fortran_array(Cnl_l4, n, l)

    print (Anm_l4)

    Anm: i16[n, m] = load_lpython_array_from_c_fortran_array(Anm_l4, n, m)
    # Issue 2497 spot_print(Anm)
    print(Anm)
    Bml: i16[m, l] = load_lpython_array_from_c_fortran_array(Bml_l4, m, l)
    # print(Bml)
    Cnl: i16[n, l] = load_lpython_array_from_c_fortran_array(Cnl_l4, n, l)
    # print(Cnl)

    # Temporaries (TODO: get rid of them, as indicated by proposed syntax below)
    B1l: i16[1, l] = empty((1, l), dtype=int16)
    T1l: i16[1, l] = empty((1, l), dtype=int16)

    VR_SIZE: i32 = 32_768
    k: i32
    A_ik: i16
    jj: i32
    ii: i32
    i: i32
    ww: i32  # "ww" is short for "workaround_index."
    for jj in range(0, l, VR_SIZE):  # each VR-col chunk in B and C
        for ii in range(0, n, M2):  # each M2 block in A cols and B rows
            for i in range(0, M2):  # zero-out rows of C
                # Due to Issue 2496, I cannot pass an array to a function
                # clear_row(Cnl, i + ii, l)
                # Due to Issue 2500, I cannot broadcast a constant
                # Cnl[i + ii, :] = 0
                for ww in range(0, l):
                    Cnl[i + ii, ww] = i16(0)
                pass
            for k in range(0, m):  # rows of B
                # Issues 2496 and 2500 prevent the desirable form and workaround
                # B_1l[0, :] = B_ml[k, :]
                for ww in range(0, l):
                    B1l[0, ww] = Bml[k, ww]
                for i in range(0, M2):
                    A_ik = Anm[i + ii, k]
                    # broadcast a single element of A (why? might have a SIMD vector register for T1l)
                    # T1l[0, :] = A_ik
                    for ww in range(0, l):
                        T1l[0, ww] = A_ik
                    # pointwise (Hadamard) product:
                    # T1l[0, :] = np.multiply(B1l[0, :], T1l[0, :])
                    for ww in range(0, l):
                        T1l[0, ww] *= B1l[0, ww]
                    # Accumulated outer product:
                    # Cnl[i + ii, :] += T1l[0, :]
                    for ww in range(0, l):
                        Cnl[i + ii, ww] += T1l[0, ww]
                    # optimization without the temporaries
                    ######## Cnl[i + ii, :] += Bml[k, :] * A_ik
                    pass

    print("Expect:")
    print("[[ 5  8 11 ... 20 23 26],")
    print(" [ 8 14 20 ... 38 44 50],")
    print(" [11 20 29 ... 56 65 74], ...")
    print("")
    print(" [ 8 14 20 ... 38 44 50],")
    print(" [11 20 29 ... 56 65 74],")
    print(" [14 26 38 ... 74 86 98]]")
    print("")
    print("Actual:")
    for ww in range(0, 3):
        print(Cnl[ww, 0], Cnl[ww, 1], Cnl[ww, 2], "...", Cnl[ww, l - 3], Cnl[ww, l - 2], Cnl[ww, l - 1])
    print("...")
    for ww in range(n-3, n):
        print(Cnl[ww, 0], Cnl[ww, 1], Cnl[ww, 2], "...", Cnl[ww, l - 3], Cnl[ww, l - 2], Cnl[ww, l - 1])
    # print(Cnl)
    # for ww in range(0, l):
    #     T1l[0, ww] = Cnl[0, ww]
    # print(T1l)
    return 0


if __name__ == "__main__":
    main()
