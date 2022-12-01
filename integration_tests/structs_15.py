from ltypes import i32, i16, i64, CPtr, dataclass, ccall, Pointer, c_p_pointer, sizeof

@dataclass
class A:
    x: i32
    y: i16

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass

def add_A_members(Ax: i32, Ay: i16) -> i32:
    return Ax + i32(Ay)

def test_Aptr_member_passing():
    print(sizeof(A))
    a_cptr: CPtr = _lfortran_malloc(sizeof(A))
    a_ptr: Pointer[A] = c_p_pointer(a_cptr, A)
    print(a_ptr.x, a_ptr.y)
    a_ptr.x = 20
    a_ptr.y = i16(-18)
    print(a_ptr.x, a_ptr.y)
    print(add_A_members(a_ptr.x, a_ptr.y))
    assert add_A_members(a_ptr.x, a_ptr.y) == 2

test_Aptr_member_passing()
