from ltypes import (i8, dataclass, i32, f32, c32, f64, i16, i64, c64,
    ccall, CPtr, c_p_pointer, Pointer, packed, ccallable)
from numpy import empty, int8, int16, float32, complex64

@ccallable
@packed
@dataclass
class buffer_struct_packed:
    buffer8: i8[32]
    buffer1: i16[32]
    buffer2: i32
    buffer3: f32[32]
    buffer4: f64
    buffer5: c32[32]
    buffer6: c64

@ccall
def get_buffer() -> CPtr:
    pass

@ccall
def fill_buffer(buffer_cptr: CPtr):
    pass

def f():
    i: i32
    b: CPtr = get_buffer()
    pb: Pointer[buffer_struct_packed] = c_p_pointer(b, buffer_struct_packed)
    pb.buffer8 = empty(32, dtype=int8)
    pb.buffer1 = empty(32, dtype=int16)
    pb.buffer2 = i32(5)
    pb.buffer3 = empty(32, dtype=float32)
    pb.buffer4 = f64(6)
    pb.buffer5 = empty(32, dtype=complex64)
    pb.buffer6 = c64(8)
    print(pb.buffer2)
    print(pb.buffer4)
    print(pb.buffer6)
    assert pb.buffer2 == i32(5)
    assert pb.buffer4 == f64(6)
    assert pb.buffer6 == c64(8)

    fill_buffer(b)
    print(pb.buffer2)
    print(pb.buffer4)
    print(pb.buffer6)
    assert pb.buffer2 == i32(3)
    assert pb.buffer4 == f64(5)
    assert pb.buffer6 == c64(7)

    for i in range(32):
        print(pb.buffer8[i], pb.buffer1[i], pb.buffer3[i], pb.buffer5[i])
        assert pb.buffer8[i] == i8(i + 8)
        assert pb.buffer1[i] == i16(i + 1)
        assert pb.buffer3[i] == f32(i + 3)
        assert pb.buffer5[i] == c32(i + 5)

f()
