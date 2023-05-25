from lpython import Const, f64, Pointer, c_p_pointer, ccall, i64, i32, CPtr

SIZEOF_DOUBLE_IN_BYTES: Const[i64] = i64(8)

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass

def foo(xs_ptr: CPtr, length: i64) -> None:
    xs: Pointer[f64[length]] = c_p_pointer(xs_ptr, f64[length])
    xs[0] = 3.0
    xs[1] = 4.0

def main() -> None:
    length: i64 = i64(32)
    xs_ptr: CPtr = _lfortran_malloc(i32(length * SIZEOF_DOUBLE_IN_BYTES))
    foo(xs_ptr, length)
    t: Pointer[f64[:]] = c_p_pointer(xs_ptr, f64[:])
    print(t[0], t[1])
    assert t[0] == 3.0
    assert t[1] == 4.0

main()
