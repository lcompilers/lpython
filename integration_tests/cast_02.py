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

# Disable following tests
# Negative numbers in unsigned should throw errors
# TODO: Add these tests as error reference tests

# def test_03():
#     x : u32 = u32(-10)
#     print(x)
#     assert x == u32(4294967286)

#     y: u16 = u16(x)
#     print(y)
#     assert y == u16(65526)

#     z: u64 = u64(y)
#     print(z)
#     assert z == u64(65526)

#     w: u8 = u8(z)
#     print(w)
#     assert w == u8(246)

# def test_04():
#     x : u64 = u64(-11)
#     print(x)
#     # TODO: We are unable to store the following u64 in AST/R
#     # assert x == u64(18446744073709551605)

#     y: u8 = u8(x)
#     print(y)
#     assert y == u8(245)

#     z: u16 = u16(y)
#     print(z)
#     assert z == u16(245)

#     w: u32 = u32(z)
#     print(w)
#     assert w == u32(245)


def main0():
    test_01()
    test_02()
    # test_03()
    # test_04()

main0()
