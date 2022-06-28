from ltypes import i32, f32, dataclass

@dataclass
class A:
    y: f32
    x: i32

@dataclass
class B:
    a: A
    z: i32

def f(b: B):
    print(b.z, b.a.x, b.a.y)
    assert b.z == 1
    assert b.a.x == 2
    assert b.a.y == 3.0

def g():
    a: A = A(1.0, 1.0)
    b: B = B(a, 1)
    b.z = 1
    b.a.x = 2
    b.a.y = 3.0
    f(b)

g()
