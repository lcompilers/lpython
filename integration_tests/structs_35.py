from lpython import (i8, i32, i64, f32, f64,
                     dataclass
                     )
from numpy import (empty,
                   int8,
                   )

# test issue 2131

@dataclass
class Foo:
    a : i8[4] = empty(4, dtype=int8)
    dim : i32 = 4

def trinary_majority(x : Foo, y : Foo, z : Foo) -> Foo:
    foo : Foo = Foo()

    assert foo.dim == x.dim == y.dim == z.dim

    return foo


t1 : Foo = Foo()
t1.a = empty(4, dtype=int8)

t2 : Foo = Foo()
t2.a = empty(4, dtype=int8)

t3 : Foo = Foo()
t3.a = empty(4, dtype=int8)

r1 : Foo = trinary_majority(t1, t2, t3)
