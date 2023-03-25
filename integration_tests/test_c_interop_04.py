from lpython import (ccall, f32, f64, i32, i64, CPtr, pointer, Pointer,
        p_c_pointer, empty_c_void_p)
from numpy import empty, int32

# @ccall
# def sum_pi32_i32(x: CPtr) -> i32:
#     pass

def test_c_callbacks():
    xi32: i32[4]
    xi32 = empty(4, dtype=int32)
    sumi32: i32
    xi32[0] = 3
    xi32[1] = 4
    xi32[2] = 5
    xi32[3] = 6
    print(xi32[0], xi32[1], xi32[2], xi32[3])
    p: CPtr
    p = empty_c_void_p()
    p_c_pointer(pointer(xi32), p)
    print(pointer(xi32), p)
    # sumi32 =  sum_pi32_i32(p)
    # print(sumi32)
    # assert sumi32 == 18

test_c_callbacks()
