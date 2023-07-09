import numpy, ctypes
from lpython import (i64, i16, CPtr, c_p_pointer, Pointer, sizeof, packed,
        dataclass, ccallable, ccall, i32)

global_arrays = []


def alloc(buf_size:i64) -> CPtr:
    xs = numpy.empty(buf_size, dtype=numpy.uint8)
    global_arrays.append(xs)
    p = ctypes.c_void_p(xs.ctypes.data)
    return ctypes.cast(p.value, ctypes.c_void_p)


@ccallable
@packed
@dataclass
class S:
    a: i16
    b: i64


def main():
    p1: CPtr = alloc(sizeof(S))
    print(p1)
    p2: Pointer[S] = c_p_pointer(p1, S)
    p2.a = i16(5)
    p2.b = i64(4)
    print(p2.a, p2.b)
    assert p2.a == i16(5)
    assert p2.b == i64(4)


main()
