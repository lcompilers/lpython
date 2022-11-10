from ltypes import (ccall, f32, f64, i32, i64, CPtr, pointer, Pointer,
        p_c_pointer, empty_c_void_p)

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

@ccall
def f_pvoid_pvoid(x: CPtr) -> CPtr:
    pass

def test_c_callbacks():
    xi32: i32
    xi32 = 3
    p: CPtr
    p = empty_c_void_p()
    p_c_pointer(pointer(xi32, i32), p)
    print(pointer(xi32, i32), p)
    assert f_pi32_i32(p) == 4
    assert f_pi32_i32(f_pvoid_pvoid(p)) == 4

    xi64: i64
    xi64 = i64(3)
    p_c_pointer(pointer(xi64, i64), p)
    print(pointer(xi64, i64), p)
    assert f_pi64_i32(p) == i64(4)
    assert f_pi64_i32(f_pvoid_pvoid(p)) == i64(4)

    xf32: f32
    xf32 = f32(3.3)
    p_c_pointer(pointer(xf32, f32), p)
    print(pointer(xf32, f32), p)
    assert abs(f_pf32_i32(p) - f32(4.3)) < f32(1e-6)
    assert abs(f_pf32_i32(f_pvoid_pvoid(p)) - f32(4.3)) < f32(1e-6)

    xf64: f64
    xf64 = 3.3
    p_c_pointer(pointer(xf64, f64), p)
    print(pointer(xf64, f64), p)
    assert abs(f_pf64_i32(p)-4.3) < 1e-12
    assert abs(f_pf64_i32(f_pvoid_pvoid(p))-4.3) < 1e-12

test_c_callbacks()
