from lpython import i32, f64, i64, i16, i8, f32, dataclass, InOut, Array
from numpy import empty

@dataclass
class A:
    y: f64
    x: i32
    z: i64
    a: f32
    b: i16
    c: i8
    d: bool

def verify(s: Array[A, :], x1: i32, y1: f64, x2: i32, y2: f64):
    eps: f64 = 1e-12
    s0: A = s[0]
    print(s0.x, s0.y, s0.z, s0.a, s0.b, s0.c, s0.d)
    assert s0.x == x1
    assert abs(s0.y - y1) < eps
    assert s0.z == i64(x1)
    assert abs(s0.a - f32(y1)) < f32(1e-6)
    assert s0.b == i16(x1)
    assert s0.c == i8(x1)
    assert s0.d

    s1: A = s[1]
    print(s1.x, s1.y, s1.z, s1.a, s1.b, s1.c, s1.d)
    assert s1.x == x2
    assert abs(s1.y - y2) < eps
    assert s1.z == i64(x2)
    assert abs(s1.a - f32(y2)) < f32(1e-6)
    assert s1.b == i16(x2)
    assert s1.c == i8(x2)
    assert s1.d

def update_1(s: InOut[A]):
    s.x = 2
    s.y = 1.2
    s.z = i64(2)
    s.a = f32(1.2)
    s.b = i16(2)
    s.c = i8(2)

def update_2(s: Array[A, :]):
    s[1].x = 3
    s[1].y = 2.3
    s[1].z = i64(3)
    s[1].a = f32(2.3)
    s[1].b = i16(3)
    s[1].c = i8(3)

def g():
    y: Array[A, 2] = empty([2], dtype=A)
    y[0] = A(1.1, 1, i64(1), f32(1.1), i16(1), i8(1), True)
    y[1] = A(2.2, 2, i64(2), f32(2.2), i16(2), i8(2), True)
    verify(y, 1, 1.1, 2, 2.2)
    update_1(y[0])
    update_2(y)
    verify(y, 2, 1.2, 3, 2.3)

g()
