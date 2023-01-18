from ltypes import (i8, dataclass, i32, f32, c32, f64, i16, i64, c64,
        ccallable, packed, ccall, CPtr, p_c_pointer, empty_c_void_p, pointer,
        c_p_pointer)
from numpy import empty, int8, int16, int32, int64, float32, complex64, complex128, float64
from copy import deepcopy

@packed
@dataclass
class buffer_struct:
    i: i8
    x: i64

@ccall
def get_buffer() -> CPtr:
    pass

@ccall
def fill_buffer(buffer_cptr: CPtr):
    pass

def f():
    b: CPtr = get_buffer()
    pb: Pointer[buffer_struct] = c_p_pointer(b, buffer_struct)
    pb.x = i64(3)
    print(pb.x)
    assert pb.x == i64(3)

    fill_buffer(b)
    print(pb.x)
    assert pb.x == i64(8)


f()
