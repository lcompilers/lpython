from ltypes import i32, i16, i64, CPtr, dataclass, ccall, Pointer, c_p_pointer

@dataclass
class A:
    x: i32
    y: i16

@ccall
def cmalloc(size: i64) -> CPtr:
    pass

@ccall
def is_null(ptr: CPtr) -> i32:
    pass

def add_A_members(Ax: i32, Ay: i16) -> i32:
    return Ax + i32(Ay)

def test_A_member_passing():
    array_cptr: CPtr = cmalloc(sizeof(A) * i64(10))
    assert not bool(is_null(array_cptr)), "Failed to allocate array on memory"
    array_ptr: Pointer[A[:]]
    c_p_pointer(array_cptr, array_ptr)
    i: i32; sum_A_members: i32
    for i in range(10):
        array_ptr[i] = A(i, i16(i + 1))

    for i in range(5):
        a: A = array_ptr[i]
        sum_A_members = add_A_members(a.x, a.y)
        assert a.x == i
        assert a.y == i16(i + 1)
        print(sum_A_members)
        assert sum_A_members == 2*i + 1

    for i in range(6, 10):
        sum_A_members = add_A_members(array_ptr[i].x, array_ptr[i].y)
        assert array_ptr[i].x == i
        assert array_ptr[i].y == i16(i + 1)
        print(sum_A_members)
        assert sum_A_members == 2*i + 1

test_A_member_passing()
