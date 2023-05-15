from lpython import dataclass, i32, f64, u64
from numpy import array

@dataclass
class Foo:
    x: i32
    y: i32

@dataclass
class Foo2:
    p: f64
    q: i32
    r: u64

def main0() -> None:
    foos: Foo[2] = array([Foo(1, 2), Foo(3, 4)])
    print(foos[0].x, foos[0].y, foos[1].x, foos[1].y)

    assert foos[0].x == 1
    assert foos[0].y == 2
    assert foos[1].x == 3
    assert foos[1].y == 4

def main1() -> None:
    foos2: Foo2[3] = array([Foo2(-2.3, 42, u64(3)), Foo2(45.5, -3, u64(10001)), Foo2(1.0, -101, u64(100))])
    i: i32
    for i in range(3):
        print(foos2[i].p, foos2[i].q, foos2[i].r)

    eps: f64
    eps = 1e-12
    assert abs(foos2[0].p - (-2.3)) <= eps
    assert foos2[0].q == 42
    assert foos2[0].r == u64(3)
    assert abs(foos2[1].p - (45.5)) <= eps
    assert foos2[1].q == -3
    assert foos2[1].r == u64(10001)
    assert abs(foos2[2].p - (1.0)) <= eps
    assert foos2[2].q == -101
    assert foos2[2].r == u64(100)

main0()
main1()
