from ltypes import i16, i32, i64, f32, f64, c32, c64
from numpy import empty, int16, int32, int64, float32, float64, complex64, complex128
from enum import Enum

class ArraySizes(Enum):
    SIZE_3: i32 = 3
    SIZE_10: i32 = 10

def accept_i16_array(xi16: i16[:]) -> i16:
    xi16[2] = i16(32)
    return xi16[2]

def accept_i32_array(xi32: i32[:]) -> i32:
    xi32[1] = 32
    return xi32[1]

def accept_i64_array(xi64: i64[:]) -> i64:
    xi64[1] = i64(64)
    return xi64[1]

def accept_f32_array(xf32: f32[:]) -> f32:
    xf32[1] = f32(32.0)
    return xf32[1]

def accept_f64_array(xf64: f64[:]) -> f64:
    xf64[0] = 64.0
    return xf64[0]

def declare_arrays():
    ai16: i16[ArraySizes.SIZE_3.value] = empty(ArraySizes.SIZE_3.value, dtype=int16)
    ai32: i32[ArraySizes.SIZE_3] = empty(ArraySizes.SIZE_3.value, dtype=int32)
    ai64: i64[10] = empty(10, dtype=int64)
    af32: f32[3] = empty(3, dtype=float32)
    af64: f64[ArraySizes.SIZE_10] = empty(10, dtype=float64)
    ac32: c32[ArraySizes.SIZE_3] = empty(3, dtype=complex64)
    ac64: c64[10] = empty(10, dtype=complex128)
    print(accept_i16_array(ai16))
    print(accept_i32_array(ai32))
    print(accept_i64_array(ai64))
    print(accept_f32_array(af32))
    print(accept_f64_array(af64))

declare_arrays()
