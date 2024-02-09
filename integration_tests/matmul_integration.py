import numpy
from numpy import array, empty, int16
from lpython import (i16, i32, ccallback, c_p_pointer, Pointer, u64, CPtr, i64,
                     ccall, sizeof, Array, Allocatable, TypeVar, Const)


# https://numpy.org/devdocs/reference/typing.html
# from numpy.typing import NDArray


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


def spot_print(Anl: i16[:, :], n: i32, l: i32) -> None:
    j: i32
    for j in range(0, 3):
        spot_print_row(Anl, l, j)
    print("...")
    for j in range(n - 3, n):
        spot_print_row(Anl, l, j)


def spot_print_row(Anl: i16[:, :], cols: i32, row: i32):
    if (cols > 3):
        print(Anl[row, 0], Anl[row, 1], Anl[row, 2], "...",
              Anl[row, cols - 3], Anl[row, cols - 2], Anl[row, cols - 1])
    else:
        print(Anl[row, 0], Anl[row, 1], Anl[row, 2])


def clear_row(a: i16[:, :], row: i32, cols: i32) -> None:
    # Due to Issue 2500, I cannot broadcast a constant.
    j: i32
    for j in range(cols):
        a[row, j] = i16(0)


def broadcast_i16_row(
        a: i16[:, :], row: i32, val: i16,
        cols: i32) -> None:
    # Due to Issue 2500, I cannot broadcast a constant.
    j: i32
    for j in range(cols):
        a[row, j] = i16(val)


def broadcast_copy_row(
        dest: i16[:, :], dest_row: i32,
        src: i16[:, :], src_row: i32,
        cols: i32) -> None:
    # Due to Issue 2500, I cannot broadcast.
    j: i32
    for j in range(cols):
        dest[dest_row, j] = src[src_row, j]


def hadamard_product_in_place_row(
        dest: i16[:, :], dest_row: i32,
        src: i16[:, :], src_row: i32,
        cols: i32) -> None:
    j: i32
    for j in range(cols):
        dest[dest_row, j] *= src[src_row, j]


def accumulate_in_place_row(
        dest: i16[:, :], dest_row: i32,
        src: i16[:, :], src_row: i32,
        cols: i32) -> None:
    j: i32
    for j in range(cols):
        dest[dest_row, j] += src[src_row, j]


def accumulate_in_place_outer_product_row(
        dest: i16[:, :], dest_row: i32,
        src1: i16[:, :], src1_row: i32,
        src2: i16[:, :],
        cols: i32) -> None:
    ww: i32
    for ww in range(0, cols):
        dest[dest_row, ww] += src1[src1_row, ww] * src2[dest_row, src1_row]



def print_expected():
    print("\nExpected result:")
    print("[[ 5  8 11 ... 20 23 26],")
    print(" [ 8 14 20 ... 38 44 50],")
    print(" [11 20 29 ... 56 65 74], ...")
    print("")
    print(" [ 8 14 20 ... 38 44 50],")
    print(" [11 20 29 ... 56 65 74],")
    print(" [14 26 38 ... 74 86 98]]")
    print("")


def main() -> i32:

    # "Const" lets these appear in type declarations such as i16[n, m]
    n  : Const[i32] = 15
    m  : Const[i32] = 3
    l  : Const[i32] = 32_768

    # M1 : i32 = 1  # Unused
    M2 : i32 = 5  # Issue 2499 -- can't be Const

    Anm_l4 : CPtr = _lfortran_malloc((n * m) * i32(sizeof(i16)))
    Bml_l4 : CPtr = _lfortran_malloc((m * l) * i32(sizeof(i16)))
    Cnl_l4 : CPtr = _lfortran_malloc((n * l) * i32(sizeof(i16)))

    init_c_fortran_array(Anm_l4, n, m, 11)
    init_c_fortran_array(Bml_l4, m, l, 13)
    zero_c_fortran_array(Cnl_l4, n, l)

    print("\nInputs:")

    Anm: i16[n, m] = load_lpython_array_from_c_fortran_array(Anm_l4, n, m)
    print("Anm[", n, ",", m, "]")
    spot_print(Anm, n, m)
    Bml: i16[m, l] = load_lpython_array_from_c_fortran_array(Bml_l4, m, l)
    print("Bml[", m, ",", l, "]")
    spot_print(Bml, m, l)
    Cnl: i16[n, l] = load_lpython_array_from_c_fortran_array(Cnl_l4, n, l)
    print("Cnl[", n, ",", l, "]")
    spot_print(Cnl, n, l)

    print_expected()

    VR_SIZE: i32 = 32_768
    ww: i32  # "ww" is short for "workaround_index."

    print("\nhand-blocked accumulated outer product; block size = M2 =", M2)
    hand_optimized_to_remove_temporaries(Anm, Bml, Cnl, n, m, l, VR_SIZE, M2)

    print("\nActual result:")
    spot_print(Cnl, n, l)

    with_liberal_use_of_temporaries(Anm, Bml, Cnl, n, m, l, VR_SIZE, M2)

    print("\nActual result:")
    spot_print(Cnl, n, l)

    unblocked_accumulated_outer_product(Anm, Bml, Cnl, n, m, l)

    print("\nActual result:")
    spot_print(Cnl, n, l)

    return 0


def unblocked_accumulated_outer_product(
        Anm: i16[:, :], Bml: i16[:, :], Cnl: i16[:, :],
        n: i32, m: i32, l: i32):
    print("\nunblocked, naive Accumulated Outer Product for reference")
    i: i32
    k: i32
    ww: i32
    for i in range(0, n):
        clear_row(Cnl, i, l)
        for k in range(0, m):  # rows of B
            accumulate_in_place_outer_product_row(Cnl, i, Bml, k, Anm, l)


def with_liberal_use_of_temporaries(
        Anm: i16[:, :], Bml: i16[:, :], Cnl: i16[:, :],
        n: i32, m: i32, l: i32, VR_SIZE: i32, M2: i32):
    k: i32
    jj: i32
    ii: i32
    i: i32
    ww: i32
    print("\nliberal usage of temporaries")
    # Temporaries (TODO: get rid of them, as indicated by proposed syntax below)
    B1l: i16[1, l] = empty((1, l), dtype=int16)
    T1l: i16[1, l] = empty((1, l), dtype=int16)
    for jj in range(0, l, VR_SIZE):  # each VR-col chunk in B and C
        for ii in range(0, n, M2):  # each M2 block in A cols and B rows
            for i in range(0, M2):  # Zero-out rows of C.
                clear_row(Cnl, i + ii, l)
            for k in range(0, m):  # rows of B
                # B_1l[0, :] = B_ml[k, :]
                broadcast_copy_row(B1l, 0, Bml, k, l)
                for i in range(0, M2):
                    # T1l[0, :] = Anm[i + ii, k]
                    broadcast_i16_row(T1l, 0, Anm[i + ii, k], l)
                    # T1l[0, :] = np.multiply(B1l[0, :], T1l[0, :])
                    hadamard_product_in_place_row(T1l, 0, B1l, 0, l)
                    # Cnl[i + ii, :] += T1l[0, :]
                    accumulate_in_place_row(Cnl, i + ii, T1l, 0, l)


def hand_optimized_to_remove_temporaries(
        Anm: i16[:, :], Bml: i16[:, :], Cnl: i16[:, :],
        n: i32, m: i32, l: i32, VR_SIZE: i32, M2: i32):
    k: i32
    jj: i32
    ii: i32
    i: i32
    ww: i32
    print("\noptimized by hand to remove temporaries")
    for jj in range(0, l, VR_SIZE):  # each VR-col chunk in B and C
        for ii in range(0, n, M2):  # each M2 block in A cols and B rows
            for i in range(0, M2):  # Zero-out rows of C.
                clear_row(Cnl, i + ii, l)
            for k in range(0, m):  # rows of B
                for i in range(0, M2):
                    accumulate_in_place_outer_product_row(
                        Cnl, i + ii, Bml, k, Anm, l)


if __name__ == "__main__":
    main()
