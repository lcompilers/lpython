from ltypes import i32, f32, dataclass

@dataclass
class C:
    cz: f32

@dataclass
class B:
    z: i32
    bc: C

@dataclass
class A:
    y: f32
    x: i32
    b: B


def f(a: A):
    print(a.x)
    print(a.y)
    print(a.b.z)

def g():
    x: A
    x = A(3.25, 3, B(71, C(4.0)))
    f(x)
    assert x.x == 3
    assert x.y == 3.25
    assert x.b.z == 71
    assert x.b.bc.cz == 4.0

g()
