from lpython import dataclass, i32

@dataclass
class Foo:
     x: i32
     y: i32


def main0() -> None:
    x: list[Foo]
    y: Foo = Foo(0, 1)
    z: Foo = Foo(1, 2)
    x = [y, z]
    i: i32 = 0
    for y in x:
        assert y.x == i
        assert y.y == i+1
        i += 1

main0()
