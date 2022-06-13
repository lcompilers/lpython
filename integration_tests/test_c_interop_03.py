from ltypes import ccall, f32, f64, i32, i64, CPtr, pointer, Pointer, p_c_pointer

@ccall
def f_pi32_i32(x: CPtr) -> i32:
    pass

def test_c_callbacks():
    xi32: i32
    xi32 = 3
    pxi32: Pointer[i32]
    pxi32 = pointer(xi32)
    p: CPtr
#    p_c_pointer(p, pxi32)
#    assert f_pi32_i32(p) == 4

test_c_callbacks()
