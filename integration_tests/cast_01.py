from lpython import i32, u8, u32, dataclass
from numpy import empty, uint8

@dataclass
class LPBHV_small:
    dim : i32 = 4
    a : u8[4] = empty(4, dtype=uint8)

def main0():
    lphv_small : LPBHV_small = LPBHV_small()
    i: i32
    for i in range(4):
        lphv_small.a[i] = u8(10 + i)
        elt: u32 = u32(lphv_small.a[i])
        print(elt)
        assert elt == u32(10 + i)

main0()
