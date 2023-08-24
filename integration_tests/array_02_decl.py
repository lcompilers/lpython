from lpython import i32, i64, f32, f64, c32, c64
from numpy import empty, int32, int64, float32, float64, complex64, complex128

def accept_multidim_i32_array(xi32: i32[:, :]) -> i32:
    return xi32[0, 0]

def accept_multidim_i64_array(xi64: i64[:, :, :]) -> i64:
    return xi64[9, 9, 9]

def accept_multidim_f32_array(xf32: f32[:]) -> f32:
    return xf32[0]

def accept_multidim_f64_array(xf64: f64[:, :]) -> f64:
    return xf64[0, 1]

def declare_arrays():
    ai32: i32[3, 3] = empty([3, 3], dtype=int32)
    ai64: i64[10, 10, 10] = empty([10, 10, 10], dtype=int64)
    af32: f32[3] = empty(3, dtype=float32)
    af64: f64[10, 4] = empty([10, 4], dtype=float64)
    ac32: c32[3, 5, 99] = empty([3, 5, 99], dtype=complex64)
    ac64: c64[10, 13, 11, 16] = empty([10, 13, 11, 16], dtype=complex128)
    print(accept_multidim_i32_array(ai32))
    print(accept_multidim_i64_array(ai64))
    print(accept_multidim_f32_array(af32))
    print(accept_multidim_f64_array(af64))


declare_arrays()
