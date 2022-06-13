from ltypes import ccall, f32, f64, i32, i64, CPtr, pointer, Pointer, p_c_pointer

@ccall
def f_pi32_i32(x: CPtr) -> i32:
    pass

def test_c_callbacks():
    xi32: i32
    xi32 = 3
    p: CPtr
    p_c_pointer(pointer(xi32), p)
    print(pointer(xi32), p)
    assert f_pi32_i32(p) == 4

test_c_callbacks()
