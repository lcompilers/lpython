from lpython import i32, dataclass, Array
from numpy import empty

@dataclass
class Foo:
     x: i32
     y: i32

def init(foos: Array[Foo, :]) -> None:
    foos[0] = Foo(5, 21)

def main0() -> None:
    foos: Array[Foo, 1] = empty(1, dtype=Foo)
    init(foos)
    print("foos[0].x =", foos[0].x)

    assert foos[0].x == 5
    assert foos[0].y == 21

main0()
