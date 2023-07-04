from lpython import (i8, i32, dataclass)
from numpy import empty, int8

# test issue-2088

@dataclass
class LPBHV_small:
    dim : i32 = 4
    a : i8[4] = empty(4, dtype=int8)


def g() -> None:
    l2 : LPBHV_small = LPBHV_small(
        4,
        [i8(214), i8(157), i8(3), i8(146)])

g()
