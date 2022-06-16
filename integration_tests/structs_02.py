from ltypes import i32, f32, dataclass, CPtr, Pointer, c_p_pointer, pointer

@dataclass
class A:
    x: i32
    y: f32

@ccallable
def f(a: CPtr) -> None:
    x: i32
    y: f32
    a1: A
    a2: Pointer[A]
    a1 = A(3, 3.25)
    a2 = pointer(a1)
    print(a2, pointer(a1))
    x = a2.x
    y = a2.y
    assert x == 3
    assert y == 3.25
    c_p_pointer(a, a2)
    print(a, a2, pointer(a1))

def g():
    b: CPtr
    f(b)

g()
