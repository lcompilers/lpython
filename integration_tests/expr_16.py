from lpython import f64, Pointer, c_p_pointer, ccall, i32, CPtr, sizeof

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass

def foo(xs_ptr: CPtr, length: i32) -> None:
    xs: Pointer[f64[length]] = c_p_pointer(xs_ptr, f64[length])
    xs[0] = 3.0
    xs[1] = 4.0

def main() -> None:
    length: i32 = 32
    xs_ptr: CPtr = _lfortran_malloc(length * i32(sizeof(f64)))
    foo(xs_ptr, length)
    t: Pointer[f64[:]] = c_p_pointer(xs_ptr, f64[:])
    print(t[0], t[1])
    assert t[0] == 3.0
    assert t[1] == 4.0

main()
