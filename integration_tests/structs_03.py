from ltypes import i32, f32, dataclass, Pointer, pointer

@dataclass
class A:
    x: i32
    y: f32

def f(pa: Pointer[A]):
    print(pa.x)
    print(pa.y)

def g():
    x: A = A(3, 3.25)
    xp: Pointer[A] = pointer(x)
    assert xp.x == 3
    assert xp.y == 3.25
    xp.x = 5
    xp.y = 5.5
    f(xp)

g()
