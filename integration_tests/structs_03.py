from ltypes import i32, f32, dataclass, Pointer, pointer

@dataclass
class A:
    x: i32
    y: f32

def f(pa: Pointer[A]):
    print(pa.x)
    print(pa.y)

def g():
    x: A = A(3, 3.5)
    px: Pointer[A]
    px = pointer(x)
    px.x = 5
    px.y = 5.5
    f(px)

g()
