from ltypes import i32, i16, i8, i64, CPtr, dataclass, ccall, Pointer, c_p_pointer, sizeof

@dataclass
class A:
    x: i16
    y: i8

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass

@ccall
def _lfortran_memset(cptr: CPtr, value: i32, size: i32):
    pass

def add_A_members(Ax: i16, Ay: i8) -> i16:
    return Ax + i16(Ay)

def test_Aptr_member_passing():
    print(sizeof(A))

    a_cptr: CPtr = _lfortran_malloc(i32(sizeof(A)))
    _lfortran_memset(a_cptr, 2, i32(sizeof(A)))
    b_cptr: CPtr = _lfortran_malloc(i32(sizeof(A)))
    _lfortran_memset(b_cptr, 6, i32(sizeof(A)))

    a_ptr: Pointer[A] = c_p_pointer(a_cptr, A)
    b_ptr: Pointer[A] = c_p_pointer(b_cptr, A)
    print(a_ptr.x, a_ptr.y)
    print(b_ptr.x, b_ptr.y)
    assert a_ptr.x * i16(3) == b_ptr.x
    assert a_ptr.y * i8(3) == b_ptr.y

    a_ptr.y = i8(-18)
    assert a_ptr.x * i16(3) == b_ptr.x
    a_ptr.x = i16(20)
    print(a_ptr.x, a_ptr.y)
    print(add_A_members(a_ptr.x, a_ptr.y))
    assert add_A_members(a_ptr.x, a_ptr.y) == i16(2)

test_Aptr_member_passing()
