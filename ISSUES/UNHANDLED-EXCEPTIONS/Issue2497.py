import numpy
from numpy import array, empty, int16
from lpython import (i16, i32, ccallback, c_p_pointer, Pointer, u64, CPtr, i64,
                     ccall, sizeof, Array, Allocatable, TypeVar, Const)


@ccall
def _lfortran_malloc(size : i32) -> CPtr:
    """Borrow from bindc_07.py in integration_tests."""
    pass


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


def spot_print_lpython_array(a: i16[:], n: i32, m: i32) -> None:
    pass


def main() -> i32:

    # "Const" lets these appear in type declarations such as i16[n, m]
    n  : Const[i32] = 15
    m  : Const[i32] = 3

    Anm_l4 : CPtr = _lfortran_malloc((n * m) * i32(sizeof(i16)))

    Anm: i16[n, m] = load_lpython_array_from_c_fortran_array(Anm_l4, n, m)
    spot_print_lpython_array(Anm, n, m)

    return 0


if __name__ == "__main__":
    main()
