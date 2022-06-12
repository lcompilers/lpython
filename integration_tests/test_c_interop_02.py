from ltypes import ccall, f32, f64, i32, i64

@ccall
def f_f64_f64(x: f64) -> f64:
    pass

@ccall
def f_f32_f32(x: f32) -> f32:
    pass

@ccall
def f_i64_i64(x: i64) -> i64:
    pass

@ccall
def f_i32_i32(x: i32) -> i32:
    pass

def test_c_callbacks():
    xf64: f64
    xf64 = 3.3
    assert abs(f_f64_f64(xf64) - (xf64+1)) < 1e-12

    xf32: f32
    xf32 = 3.3
    assert abs(f_f32_f32(xf32) - (xf32+1)) < 1e-12

    xi64: i64
    xi64 = 3
    assert f_i64_i64(xi64) == 4

    xi32: i32
    xi32 = 3
    assert f_i32_i32(xi32) == 4

test_c_callbacks()
