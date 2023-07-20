from lpython import (i32, f32, dataclass, CPtr, Pointer, c_p_pointer, pointer,
        ccallable, empty_c_void_p, f64, ccall, sizeof, i64)

@ccall
def _lfortran_malloc(size: i32) -> CPtr:
    pass

def alloc(buf_size:i64) -> CPtr:
    return _lfortran_malloc(i32(buf_size))

@ccallable
@dataclass
class A:
    x: i32
    y: f32

@ccallable
def f(a: CPtr) -> None:
    x: i32
    y: f32
    a1: A = A(3, f32(3.25))
    a2: Pointer[A]
    a2 = pointer(a1)
    print(a2, pointer(a1))
    # TODO: does not work:
    #x = a2.x
    #y = a2.y
    #assert x == 3
    #assert f64(y) == 3.25
    a2 = c_p_pointer(a, A)
    print(a, a2, pointer(a1))
    print(a2.x, a2.y)
    assert a2.x == 5
    assert a2.y == f32(6.0)

def g():
    b: CPtr = alloc(sizeof(A))
    b2: Pointer[A] = c_p_pointer(b, A)
    b2.x = 5
    b2.y = f32(6)
    f(b)

g()
