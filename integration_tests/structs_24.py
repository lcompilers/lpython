from lpython import dataclass, i32, f64, u64
from numpy import array

@dataclass
class Foo:
    x: i32
    y: i32

def main0() -> None:
    foos: Foo[2] = array([Foo(y=2, x=1), Foo(x=3, y=4)])
    print(foos[0].x, foos[0].y, foos[1].x, foos[1].y)

    assert foos[0].x == 1
    assert foos[0].y == 2
    assert foos[1].x == 3
    assert foos[1].y == 4

main0()
