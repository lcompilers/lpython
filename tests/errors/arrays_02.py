from lpython import (i8, i32, dataclass)
from numpy import (empty, int8)

@dataclass
class Foo:
    a : i8[4] = empty(4, dtype=int8)
    dim : i32 = 4

def trinary_majority(x : Foo, y : Foo, z : Foo) -> Foo:
    foo : Foo = Foo()
    i : i32
    for i in range(foo.dim):
        foo.a[i] = (x.a[i] & y.a[i]) | (y.a[i] & z.a[i]) | (z.a[i] & x.a[i])
    return foo


t1 : Foo = Foo()
t1.a = empty(4, dtype=int8)

t2 : Foo = Foo()
t2.a = empty(4, dtype=int8)

t3 : Foo = Foo()
t3.a = empty(4, dtype=int8)

r1 : Foo = trinary_majority(t1, t2, t3)

assert r1.a == t1.a
