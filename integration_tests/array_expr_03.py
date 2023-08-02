from lpython import i8, i32, dataclass
from numpy import empty, int8, array


@dataclass
class LPBHV_small:
    dim: i32 = 4
    a: i8[4] = empty(4, dtype=int8)


def g():
    l2: LPBHV_small = LPBHV_small(4, array([127, -127, 3, 111], dtype=int8))

    print(l2.dim)
    assert l2.dim == 4

    print(l2.a[0], l2.a[1], l2.a[2], l2.a[3])
    assert l2.a[0] == i8(127)
    assert l2.a[1] == i8(-127)
    assert l2.a[2] == i8(3)
    assert l2.a[3] == i8(111)


g()
