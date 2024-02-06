import numpy
from numpy import array
from lpython import (i16, i32, ccallback, c_p_pointer, Pointer, u64, CPtr, i64,
                     ccall, sizeof)

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


def numpy_side_by_side(n: i32, m: i32, l: i32, M1: i32, M2: i32,
                       A: CPtr, B: CPtr, C: CPtr) -> \
        None: ######## NDArray[numpy.int16]:
    VR_SIZE: i32 = 32_768

    # In the primary example, n = 15, m = 3, l = 32_768,
    # M1 = 1, M2 = 5

    # source GSI L4 arrays
    pA_nm: Pointer[i16[:]] = c_p_pointer(A, i16[:], array([n * m]))
    pB_ml: Pointer[i16[:]] = c_p_pointer(B, i16[:], array([m * l]))

    print(pA_nm[0])
    assert pA_nm[0] == i16(0)

    pA_nm[0] = i16(32_767)
    assert pA_nm[0] == i16(0x7FFF)
    print(pA_nm[0])

    pA_nm[0] += i16(1)
    assert pA_nm[0] == i16(-32_768)
    print(pA_nm[0])

    # source numpy arrays
    ######## A_nm: NDArray[numpy.int16] = numpy.zeros((n, m), dtype=numpy.int16)
    ######## for row in range(n):
    ########     A_nm[row,:] = pA_nm[(row * m):((row + 1) * m)]
    # A_nm: Array[i16, n, m]
    # row : i32
    # for row in range(n):
    #     col : i32
    #     for col in range(m):
    #         A_nm[row, col] = pA_nm[(row * m):((row * m) + col)]

    ######## B_ml: NDArray[numpy.int16] = numpy.zeros((m, l), dtype=numpy.int16)
    ######## for row in range(m):
    ########     B_ml[row,:] = pB_ml[(row * l):((row + 1) * l)]

    # # destination numpy array
    ######## C_nl: NDArray[numpy.int16] = numpy.zeros((n, l), dtype=numpy.int16)

    # destination GSI L4 array
    pC_nl: Pointer[i16[:]] = c_p_pointer(C, i16[:], array([n * l]))

    # First, accumulate outer product without blocking. This is
    # the code we would -ultimately- like to compile. Notice that
    # all GSI-specific L1, L4, MMB are hidden.

    k: i32
    ######## for k in range(0, m):
    ########     C_nl += numpy.outer(A_nm[:,k], B_ml[k,:])
    ########     pass

    # expect
    # [[ 5  8 11 ... 20 23 26],
    #  [ 8 14 20 ... 38 44 50],
    #  [11 20 29 ... 56 65 74], ...
    #
    #  [ 8 14 20 ... 38 44 50],
    #  [11 20 29 ... 56 65 74],
    #  [14 26 38 ... 74 86 98]]
    set_breakpoint_here_and_inspect_C_nl : i32 = 0

    # Second, with explicit blocking. This is a stepping-stone
    # for our back-end. Notice that L1 and MMB are hidden.

    # T_1l: NDArray[numpy.int16] = numpy.zeros((1, l), dtype=numpy.int16)
    # B_1l: NDArray[numpy.int16] = numpy.zeros((1, l), dtype=numpy.int16)
    A_ik: i16
    jj: i32
    ii: i32
    i: i32
    for jj in range(0, l, VR_SIZE):  # each VR-col chunk in B and C
        for ii in range(0, n, M2):  # each M2 block in A cols and B rows
            for i in range(0, M2):  # zero-out rows of C
                ######## C_nl[i + ii, :] = 0
                pass
            for k in range(0, m):  # rows of B
                # B_1l[0, :] = B_ml[k, :]
                for i in range(0, M2):
                    ######## A_ik = A_nm[i + ii, k]
                    # broadcast a single element of A
                    # T_1l[0, :] = A_ik
                    # pointwise (Hadamard) product:
                    # T_1l[0, :] = np.multiply(B_1l[0, :], T_1l[0, :])
                    # C_nl[i + ii, :] += T_1l[0, :]
                    # optimization without the temporaries
                    ######## C_nl[i + ii, :] += B_ml[k, :] * A_ik
                    pass

    set_breakpoint_here_and_inspect_C_nl = 0

    ######## return C_nl


@ccall
def _lfortran_malloc(size : i32) -> CPtr:
    """borrowed from bindc_07.py in integration_tests"""
    pass


def main():
    n  : i32 = 15
    m  : i32 = 3
    l  : i32 = 32_768
    M1 : i32 = 1
    M2 : i32 = 5
    Anm_l4 : CPtr = _lfortran_malloc( (n * m) * i32(sizeof(i16)) )
    Bml_l4 : CPtr = _lfortran_malloc( (m * l) * i32(sizeof(i16)) )
    Cnl_l4 : CPtr = _lfortran_malloc( (n * l) * i32(sizeof(i16)) )
    numpy_side_by_side(n, m, l, M1, M2, Anm_l4, Bml_l4, Cnl_l4)
    print ("hello, world!")


if __name__ == "__main__":
    main()
