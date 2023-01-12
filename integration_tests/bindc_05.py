from ltypes import CPtr, empty_c_void_p, i32, f32, dataclass, Pointer, ccall, p_c_pointer, pointer

@dataclass
class Void:
    data: CPtr

@ccall
def trunc_custom(value: Pointer[CPtr]) -> CPtr:
    pass

@ccall
def print_value(value: CPtr):
    pass

def test_trunc():
    float_cptr: CPtr = empty_c_void_p()
    float_obj: f32 = f32(1.3)

    p_c_pointer(pointer(float_obj), float_cptr)
    voidobj: Void = Void(float_cptr)
    voidobj.data = trunc_custom(pointer(voidobj.data))

    print_value(voidobj.data)

test_trunc()
