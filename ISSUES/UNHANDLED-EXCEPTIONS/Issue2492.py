from numpy import empty, int16

from lpython import (i16, i32, CPtr, ccall, sizeof)


@ccall
def _lfortran_malloc(size : i32) -> CPtr:
    """borrowed from bindc_07.py in integration_tests"""
    pass


def main():
    n  : i32 = 15
    m  : i32 = 3

    # Emulate getting stuff from the C side.
    Anm_l4 : CPtr = _lfortran_malloc( (n * m) * i32(sizeof(i16)) )
    A_nm: i16[n, m] = empty((n, m), dtype=int16)


if __name__ == "__main__":
    main()
