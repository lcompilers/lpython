from ltypes import i32, i64, f32, f64, c32, c64
from numpy import empty

def accept_i32_array(xi32: i32[:]) -> i32:
    xi32[0] = 32
    return xi32[0]

def accept_i64_array(xi64: i64[:]) -> i64:
    xi64[0] = 64
    return xi64[0]

def accept_f32_array(xf32: f32[:]) -> f32:
    xf32[0] = 32.0
    return xf32[0]

def accept_f64_array(xf64: f64[:]) -> f64:
    xf64[0] = 64.0
    return xf64[0]

def declare_arrays():
    ai32: i32[3] = empty(3)
    ai64: i64[10] = empty(10)
    af32: f32[3] = empty(3)
    af64: f64[10] = empty(10)
    ac32: c32[3] = empty(3)
    ac64: c64[10] = empty(10)
    print(accept_i32_array(ai32))
    print(accept_i64_array(ai64))
    print(accept_f32_array(af32))
    print(accept_f64_array(af64))

declare_arrays()
