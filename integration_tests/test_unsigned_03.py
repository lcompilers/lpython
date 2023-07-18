from lpython import u16, i32, u8, u16, u64, i64, u32, i8

# test issue 2174

def f():
    u: u16 = u16(32768)
    assert i32(u) == 32768
    u1: u8 = u8(23)
    assert i8(u1) == i8(23)
    assert u16(u1) == u16(23)
    assert u32(u1) == u32(23)
    assert u64(u1) == u64(23)
    print(i8(u1), u16(u1), u32(u1), u64(u1))
    assert i64(u1) == i64(23)
    assert i64(u) == i64(32768)
    assert i32(u1) == 23
    print(i64(u), i32(u))

f()
