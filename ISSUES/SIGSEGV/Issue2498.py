from numpy import array, empty, int16
from lpython import (i16, i32, ccallback, c_p_pointer, Pointer, u64, CPtr, i64,
                     ccall, sizeof, Array, Allocatable, TypeVar, Const)


rows = TypeVar("rows")
cols = TypeVar("cols")


def spot_print_lpython_array(a: i16[:], rows: i32, cols: i32) -> i16[rows, cols]:
    pass


def main() -> i32:
    print ("hello, world!")
    return 0


if __name__ == "__main__":
    main()
