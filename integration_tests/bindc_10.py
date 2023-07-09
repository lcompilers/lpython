from lpython import (i64, i16, CPtr, c_p_pointer, Pointer, sizeof, packed,
        dataclass, ccallable, ccall, i32)

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass


def alloc(buf_size:i64) -> CPtr:
    return _lfortran_malloc(i32(buf_size))


@ccallable
@packed
@dataclass
class S:
    a: i16
    b: i64


def main():
    p1: CPtr = alloc(i64(1000))
    print(p1)
    p2: Pointer[S] = c_p_pointer(p1, S)
    p2.a = i16(5)
    p2.b = i64(4)
    print(p2.a, p2.b)
    assert p2.a == i16(5)
    assert p2.b == i64(4)


main()
