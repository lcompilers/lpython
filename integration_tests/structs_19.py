from ltypes import (i8, dataclass, i32, f32, c32, f64, i16, i64, c64,
    ccall, CPtr, c_p_pointer, Pointer)

@dataclass
class buffer_struct:
    buffer8: i8
    buffer1: i32
    buffer2: f32
    buffer3: c32
    buffer4: f64
    buffer5: i16
    buffer6: i64
    buffer7: c64

@ccall
def get_buffer() -> CPtr:
    pass

@ccall
def fill_buffer(buffer_cptr: CPtr):
    pass

def f():
    b: CPtr = get_buffer()
    pb: Pointer[buffer_struct] = c_p_pointer(b, buffer_struct)
    pb.buffer8 = i8(3)
    pb.buffer1 = i32(4)
    pb.buffer2 = f32(5)
    pb.buffer3 = c32(9)
    pb.buffer4 = f64(6)
    pb.buffer5 = i16(7)
    pb.buffer6 = i64(8)
    pb.buffer7 = c64(10)
    print(pb.buffer8)
    print(pb.buffer1)
    print(pb.buffer2)
    print(pb.buffer3)
    print(pb.buffer4)
    print(pb.buffer5)
    print(pb.buffer6)
    print(pb.buffer7)
    assert pb.buffer8 == i8(3)
    assert pb.buffer1 == i32(4)
    assert pb.buffer2 == f32(5)
    assert pb.buffer3 == c32(9)
    assert pb.buffer4 == f64(6)
    assert pb.buffer5 == i16(7)
    assert pb.buffer6 == i64(8)
    assert pb.buffer7 == c64(10)

    fill_buffer(b)
    print(pb.buffer8)
    print(pb.buffer1)
    print(pb.buffer2)
    print(pb.buffer3)
    print(pb.buffer4)
    print(pb.buffer5)
    print(pb.buffer6)
    print(pb.buffer7)
    assert pb.buffer8 == i8(8)
    assert pb.buffer1 == i32(9)
    assert pb.buffer2 == f32(10)
    assert pb.buffer3 == c32(14) + c32(15j)
    assert pb.buffer4 == f64(11)
    assert pb.buffer5 == i16(12)
    assert pb.buffer6 == i64(13)
    assert pb.buffer7 == c64(16) + c64(17j)


f()
