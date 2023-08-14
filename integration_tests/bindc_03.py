from lpython import (c_p_pointer, CPtr, pointer, i32,
                    Pointer, ccall, p_c_pointer, dataclass,
                    ccallable, empty_c_void_p, cptr_to_u64,
                    u64_to_cptr, u64)
from numpy import array

@dataclass
class ArrayWrapped:
    array: CPtr

@ccall(header="bindc_03b.h")
def g(a: CPtr, value: i32, offset_value: bool) -> None:
    pass

def gpy(a: CPtr, value: i32, offset_value: bool) -> None:
    g(a, value, offset_value)

@ccall(header="bindc_03b.h")
def get_array(size: i32) -> CPtr:
    pass

@ccallable
def f(q_void: CPtr) -> None:
    i: i32
    el: i32
    q: Pointer[i32[:]] = c_p_pointer(q_void, i32[:], array([10]))
    for i in range(10):
        q2: CPtr
        p_c_pointer(pointer(q[i]), q2)
        gpy(q2, i * i, bool(i%2))
        # TODO: Use q[i] directly in the assert.
        el = q[i]
        print(el)
        assert el == i * i + i%2

def h(q_void: CPtr) -> None:
    i: i32
    el: i32
    q: Pointer[i32[:]] = c_p_pointer(q_void, i32[:], array([10]))
    for i in range(10):
        # TODO: Use q[i] directly in the assert.
        el = q[i]
        print(el)
        assert el == i * i + i%2


@ccallable(header="_test_bindc_03_my_header.h")
def test_emit_header_ccallable() -> i32:
    i: i32 = 5
    assert i == 5
    i = i*5
    return i + 10

def run():
    a: CPtr
    array_wrapped: ArrayWrapped = ArrayWrapped(a)
    b: CPtr = empty_c_void_p()
    array_wrapped1: ArrayWrapped = ArrayWrapped(b)
    size: i32
    size = 10
    a = get_array(size)
    assert a != empty_c_void_p()
    array_wrapped.array = a
    f(array_wrapped.array)
    q: u64 = cptr_to_u64(a)
    x: CPtr
    x = u64_to_cptr(q)
    array_wrapped.array = x
    f(array_wrapped.array)
    array_wrapped1 = array_wrapped
    h(array_wrapped1.array)
    assert test_emit_header_ccallable() == 35

run()
