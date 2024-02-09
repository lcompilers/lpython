import numpy
from numpy import empty, int16
from lpython import (i16, i32, Const)


# ~~~~~~~~~~~~~~~~ ATTENTION ~~~~~~~~~~~~~~~~~~~~
#                     |
#                     v
def spot_print(a: i16[:], n: i32, l: i32) -> None:
    """Issue2497, Issue2521"""
    ww: i32
    for ww in range(0, 3):
        print(a[ww, 0], a[ww, 1], a[ww, 2], "...", a[ww, l - 3], a[ww, l - 2], a[ww, l - 1])
    print("...")
    for ww in range(n-3, n):
        print(a[ww, 0], a[ww, 1], a[ww, 2], "...", a[ww, l - 3], a[ww, l - 2], [ww, l - 1])


def main() -> i32:

    # "Const" lets these appear in type declarations such as i16[n, m]
    n  : Const[i32] = 15
    l  : Const[i32] = 32_768

    Cnl: i16[n, l] = empty((n, l), dtype=int16)

    spot_print(Cnl, i32(n), i32(l))


if __name__ == "__main__":
    main()
