from lpython import i32, f32, dataclass, CPtr, Pointer, c_p_pointer, pointer, ccallable, empty_c_void_p, f64

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
    x = a2.x
    y = a2.y
    assert x == 3
    assert f64(y) == 3.25

def g():
    b: CPtr = empty_c_void_p()
    f(b)

g()
