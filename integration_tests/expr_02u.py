from lpython import u8, u16, u32, u64

def add_u8(x: u8, y: u8) -> u8:
    return x + y

def add_u16(x: u16, y: u16) -> u16:
    return x + y

def add_u32(x: u32, y: u32) -> u32:
    return x + y

def add_u64(x: u64, y: u64) -> u64:
    return x + y

def and_u8(x: u8, y: u8) -> u8:
    return x & y

def and_u16(x: u16, y: u16) -> u16:
    return x & y

def and_u32(x: u32, y: u32) -> u32:
    return x & y

def and_u64(x: u64, y: u64) -> u64:
    return x & y

def main_u8():
    x: u8
    y: u8
    z: u8
    x = (u8(2)+u8(3))*u8(5)
    y = add_u8(x, u8(2))*u8(2)
    z = and_u8(x, y)
    assert x == u8(25)
    assert y == u8(54)
    assert z == u8(16)

def main_u16():
    x: u16
    y: u16
    z: u16
    x = (u16(2)+u16(3))*u16(5)
    y = add_u16(x, u16(2))*u16(2)
    z = and_u16(x, y)
    assert x == u16(25)
    assert y == u16(54)
    assert z == u16(16)

def main_u32():
    x: u32
    y: u32
    z: u32
    x = (u32(2)+u32(3))*u32(5)
    y = add_u32(x, u32(2))*u32(2)
    z = and_u32(x, y)
    assert x == u32(25)
    assert y == u32(54)
    assert z == u32(16)

def main_u64():
    x: u64
    y: u64
    z: u64
    x = (u64(2)+u64(3))*u64(5)
    y = add_u64(x, u64(2))*u64(2)
    z = and_u64(x, y)
    assert x == u64(25)
    assert y == u64(54)
    assert z == u64(16)

main_u8()
main_u16()
main_u32()
main_u64()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()
