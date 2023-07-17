from lpython import (i8, i32,
                     dataclass
                     )
from numpy import (empty,
                   int8,
                   )

# test issue 2130

r : i8 = i8(240)

@dataclass
class Foo:
    a : i8[4] = empty(4, dtype=int8)

def trinary_majority(x : Foo, y : Foo, z : Foo) -> Foo:
    foo : Foo = Foo()
    foo.a = x.a | y.a | z.a
    return foo

def f():
    t1 : Foo = Foo()
    t1.a = empty(4, dtype=int8)
    i: i32
    for i in range(4):
        t1.a[i] = i8(i+1)

    t2 : Foo = Foo()
    t2.a = empty(4, dtype=int8)

    for i in range(4):
        t2.a[i] = i8(2*i+1)

    t3 : Foo = Foo()
    t3.a = empty(4, dtype=int8)

    for i in range(4):
        t3.a[i] = i8(3*i+1)

    r1 : Foo = trinary_majority(t1, t2, t3)

    res: list[i32] = [1, 7, 7, 15]
    for i in range(4):
        assert r1.a[i] == i8(res[i])

f()
