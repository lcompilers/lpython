from lpython import dataclass, field, i32
from numpy import array

@dataclass
class X:
    a: i32 = 123
    b: bool = True
    c: list[i32] = field(default_factory=lambda: [1, 2, 3])
    d: i32[3] = field(default_factory=lambda: array([4, 5, 6]))
    e: i32 = field(default=-5)

def main0():
    x: X = X()
    print(x)
    assert x.a == 123
    assert x.b == True
    assert x.c[0] == 1
    assert x.d[1] == 5
    assert x.e == -5
    x.c[0] = 3
    x.d[0] = 3
    print(x)
    assert x.c[0] == 3
    assert x.d[0] == 3

main0()
