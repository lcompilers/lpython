from lpython import i32, f32, f64, dataclass, field
from copy import deepcopy

@dataclass
class A:
    y: f32
    x: i32

@dataclass
class B:
    z: i32
    a: A = field(default_factory=lambda: A(f32(0.0), 0))

def f(b: B):
    print(b.z, b.a.x, b.a.y)
    assert b.z == 1
    assert b.a.x == 2
    assert f64(b.a.y) == 3.0

def g():
    a1: A = A(f32(1.0), 1)
    a2: A = A(f32(2.0), 2)
    b: B = B(1, a1)
    b.a = deepcopy(a2)
    b.z = 1
    b.a.x = 2
    b.a.y = f32(3.0)
    assert a1.x == 1
    assert f64(a1.y) == 1.0
    assert a2.x == 2
    assert f64(a2.y) == 2.0
    f(b)

g()
