from lpython import u8, u16, u32, u64
from numpy import uint8, uint16, uint32, uint64, array

def g():
    a8: u8[3] = array([127, 3, 111], dtype=uint8)
    a16: u16[3] = array([127, 3, 111], dtype=uint16)
    a32: u32[3] = array([127, 3, 111], dtype=uint32)
    a64: u64[3] = array([127, 3, 111], dtype=uint64)

    print(a8)
    print(a16)
    print(a32)
    print(a64)

    assert (a8[0] == u8(127))
    assert (a8[1] == u8(3))
    assert (a8[2] == u8(111))

    assert (a16[0] == u16(127))
    assert (a16[1] == u16(3))
    assert (a16[2] == u16(111))

    assert (a32[0] == u32(127))
    assert (a32[1] == u32(3))
    assert (a32[2] == u32(111))

    assert (a64[0] == u64(127))
    assert (a64[1] == u64(3))
    assert (a64[2] == u64(111))

g()
