from lpython import u8, u16, u32, u64

def f():

    h: u8
    h = u8(67)
    print(+h)
    assert +h == u8(67)

    i: u16
    i = u16(67)
    print(+i)
    assert +i == u16(67)

    j: u32
    j = u32(25)
    print(+j)
    assert +j == u32(25)

    k: u64
    k = u64(100000000000123)
    print(+k)
    assert +k == u64(100000000000123)

    assert -u8(0) == u8(0)
    assert -u16(0) == u16(0)
    assert -u32(0) == u32(0)
    assert -u64(0) == u64(0)

f()
