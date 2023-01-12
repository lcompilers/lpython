from ltypes import i8, dataclass, i32, f32, c32, f64, i16, i64, c64, ccallable, packed, ccall, CPtr, p_c_pointer, empty_c_void_p, pointer
from numpy import empty, int8, int16, int32, int64, float32, complex64, complex128, float64
from copy import deepcopy

@packed
@dataclass
class buffer_struct:
    buffer8: i8[32]
    buffer1: i32[32]
    buffer2: f32[32]
    buffer3: c32[32]
    buffer4: f64[32]
    buffer5: i16[32]
    buffer6: i64[32]
    buffer7: c64[32]

@ccallable
@packed
@dataclass
class buffer_struct_clink:
    buffer8: i8[32]
    buffer1: i32[32]
    buffer2: f32[32]
    buffer3: c32[32]
    buffer4: f64[32]
    buffer5: i16[32]
    buffer6: i64[32]
    buffer7: c64[32]

@ccall
def sum_buffer_i16(arr: i16[:], size: i32) -> i16:
    pass

@ccall
def fill_buffer(buffer_cptr: CPtr):
    pass

def f():
    i: i32
    buffer8_var: i8[32] = empty(32, dtype=int8)
    buffer1_var: i32[32] = empty(32, dtype=int32)
    buffer2_var: f32[32] = empty(32, dtype=float32)
    buffer3_var: c32[32] = empty(32, dtype=complex64)
    buffer4_var: f64[32] = empty(32, dtype=float64)
    buffer5_var: i16[32] = empty(32, dtype=int16)
    buffer6_var: i64[32] = empty(32, dtype=int64)
    buffer7_var: c64[32] = empty(32, dtype=complex128)
    buffer_: buffer_struct = buffer_struct(deepcopy(buffer8_var), deepcopy(buffer1_var),
                                           deepcopy(buffer2_var), deepcopy(buffer3_var),
                                           deepcopy(buffer4_var), deepcopy(buffer5_var),
                                           deepcopy(buffer6_var), deepcopy(buffer7_var))
    buffer_clink_: buffer_struct_clink = buffer_struct_clink(deepcopy(buffer8_var), deepcopy(buffer1_var),
                                                             deepcopy(buffer2_var), deepcopy(buffer3_var),
                                                             deepcopy(buffer4_var), deepcopy(buffer5_var),
                                                             deepcopy(buffer6_var), deepcopy(buffer7_var))
    buffer_clink_cptr: CPtr = empty_c_void_p()
    print(buffer_.buffer8[15])
    print(buffer_.buffer1[15])
    print(buffer_.buffer2[15])
    print(buffer_.buffer3[15])
    print(buffer_.buffer4[15])
    print(buffer_.buffer5[15])
    print(buffer_.buffer6[15])
    print(buffer_.buffer7[15])
    print(buffer_clink_.buffer8[15])
    print(buffer_clink_.buffer1[15])
    print(buffer_clink_.buffer2[15])
    print(buffer_clink_.buffer3[15])
    print(buffer_clink_.buffer4[15])
    print(buffer_clink_.buffer5[15])
    print(buffer_clink_.buffer6[15])
    print(buffer_clink_.buffer7[15])

    p_c_pointer(pointer(buffer_clink_), buffer_clink_cptr)
    fill_buffer(buffer_clink_cptr)

    for i in range(32):
        buffer_.buffer8[i] = i8(i + 1)
        buffer_.buffer1[i] = i32(i + 3)
        buffer_.buffer2[i] = f32(i + 5)
        buffer_.buffer3[i] = c32(i + 7)
        buffer_.buffer4[i] = f64(i + 9)
        buffer_.buffer5[i] = i16(i + 11)
        buffer_.buffer6[i] = i64(i + 13)
        buffer_.buffer7[i] = c64(i + 15)

    for i in range(32):
        print(i, buffer_.buffer8[i], buffer_clink_.buffer8[i])
        print(i, buffer_clink_.buffer1[i], buffer_.buffer1[i])
        print(i, buffer_clink_.buffer2[i], buffer_.buffer2[i])
        print(i, buffer_clink_.buffer3[i], buffer_.buffer3[i])
        print(i, buffer_clink_.buffer4[i], buffer_.buffer4[i])
        print(i, buffer_clink_.buffer5[i], buffer_.buffer5[i])
        print(i, buffer_clink_.buffer6[i], buffer_.buffer6[i])
        print(i, buffer_clink_.buffer7[i], buffer_.buffer7[i])
        assert buffer_.buffer8[i] == i8(i + 1)
        assert buffer_clink_.buffer8[i] == i8(i + 2)
        assert buffer_clink_.buffer8[i] - buffer_.buffer8[i] == i8(1)
        assert buffer_clink_.buffer1[i] - buffer_.buffer1[i] == i32(1)
        assert buffer_clink_.buffer2[i] - buffer_.buffer2[i] == f32(1)
        assert buffer_clink_.buffer3[i] - buffer_.buffer3[i] == c32(1)
        assert buffer_clink_.buffer4[i] - buffer_.buffer4[i] == f64(1)
        assert buffer_clink_.buffer5[i] - buffer_.buffer5[i] == i16(1)
        assert buffer_clink_.buffer6[i] - buffer_.buffer6[i] == i64(1)
        assert buffer_clink_.buffer7[i] - buffer_.buffer7[i] == c64(1)

    print(sum_buffer_i16(buffer_.buffer5, 32))
    assert sum_buffer_i16(buffer_clink_.buffer5, 32) == i16(880)

f()
