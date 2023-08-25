from lpython import u8, u16, u32, u64

def f():

    h: u8
    h = u8(5)
    print(h << u8(4), h << u8(2), h >> u8(4), h >> u8(7))
    assert h << u8(4) == u8(80)
    assert h << u8(2) == u8(20)
    assert h >> u8(4) == u8(0)
    assert h >> u8(7) == u8(0)

    i: u16
    i = u16(67)
    print(i << u16(4), i << u16(7), i >> u16(4), i >> u16(7))
    assert i << u16(4) == u16(1072)
    assert i << u16(7) == u16(8576)
    assert i >> u16(4) == u16(4)
    assert i >> u16(7) == u16(0)

    j: u32
    j = u32(25)
    print(j << u32(4), j << u32(7), j >> u32(4), j >> u32(7))
    assert j << u32(4) == u32(400)
    assert j << u32(7) == u32(3200)
    assert j >> u32(4) == u32(1)
    assert j >> u32(7) == u32(0)

    k: u64
    k = u64(100000000000123)
    print(k << u64(4), k << u64(7), k >> u64(4), k >> u64(7))
    assert k << u64(4) == u64(1600000000001968)
    assert k << u64(7) == u64(12800000000015744)
    assert k >> u64(4) == u64(6250000000007)
    assert k >> u64(7) == u64(781250000000)

f()
