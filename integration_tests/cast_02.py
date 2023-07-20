from lpython import u8, u16, u32, u64

def test_01():
    x : u32 = u32(10)
    print(x)
    assert x == u32(10)

    y: u16 = u16(x)
    print(y)
    assert y == u16(10)

    z: u64 = u64(y)
    print(z)
    assert z == u64(10)

    w: u8 = u8(z)
    print(w)
    assert w == u8(10)

def test_02():
    x : u64 = u64(11)
    print(x)
    assert x == u64(11)

    y: u8 = u8(x)
    print(y)
    assert y == u8(11)

    z: u16 = u16(y)
    print(z)
    assert z == u16(11)

    w: u32 = u32(z)
    print(w)
    assert w == u32(11)

def main0():
    test_01()
    test_02()

main0()
