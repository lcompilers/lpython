from ltypes import ccall, f32, f64, i32, i64, CPtr, pointer, Pointer, p_c_pointer

@ccall
def f_pi32_i32(x: CPtr) -> i32:
    pass

@ccall
def f_pi64_i32(x: CPtr) -> i64:
    pass

@ccall
def f_pf32_i32(x: CPtr) -> f32:
    pass

@ccall
def f_pf64_i32(x: CPtr) -> f64:
    pass

def test_c_callbacks():
    xi32: i32
    xi32 = 3
    p: CPtr
    p_c_pointer(xi32, p)
    assert f_pi32_i32(p) == 4

    xi64: i64
    xi64 = 3
    p_c_pointer(xi64, p)
    assert f_pi64_i32(p) == 4

    xf32: f32
    xf32 = 3.3
    p_c_pointer(xf32, p)
    assert abs(f_pf32_i32(p)-4.3) < 1e-6

    xf64: f64
    xf64 = 3.3
    p_c_pointer(xf64, p)
    assert abs(f_pf64_i32(p)-4.3) < 1e-12

test_c_callbacks()
