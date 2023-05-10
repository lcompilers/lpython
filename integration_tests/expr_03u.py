from lpython import u8, u16, u32, u64, i8, i32, TypeVar
from numpy import (empty, uint8, uint16, uint32, uint64, int8, int16, int32,
        int64, size)

n = TypeVar("n")
def add_i8(n: i32, x: i8[n], y: i8[n]) -> i8[n]:
    return x + y

def add_i8_loop(n: i32, x: i8[n], y: i8[n]) -> i8[n]:
    z: i8[n] = empty(n, dtype=int8)
    i: i32
    for i in range(n):
        z[i] = x[i] + y[i]
    return z

def add_u8(n: i32, x: u8[n], y: u8[n]) -> u8[n]:
    return x + y

def add_u8_loop(n: i32, x: u8[n], y: u8[n]) -> u8[n]:
    z: u8[n] = empty(n, dtype=uint8)
    i: i32
    for i in range(n):
        z[i] = x[i] + y[i]
    return z

def add_u16(n: i32, x: u16[n], y: u16[n]) -> u16[n]:
    return x + y

def add_u16_loop(n: i32, x: u16[n], y: u16[n]) -> u16[n]:
    z: u16[n] = empty(n, dtype=uint16)
    i: i32
    for i in range(n):
        z[i] = x[i] + y[i]
    return z

def add_u32(n: i32, x: u32[n], y: u32[n]) -> u32[n]:
    return x + y

def add_u32_loop(n: i32, x: u32[n], y: u32[n]) -> u32[n]:
    z: u32[n] = empty(n, dtype=uint32)
    i: i32
    for i in range(n):
        z[i] = x[i] + y[i]
    return z

def add_u64(n: i32, x: u64[n], y: u64[n]) -> u64[n]:
    return x + y

def add_u64_loop(n: i32, x: u64[n], y: u64[n]) -> u64[n]:
    z: u64[n] = empty(n, dtype=uint64)
    i: i32
    for i in range(n):
        z[i] = x[i] + y[i]
    return z

def main_i8():
    x: i8[3] = empty(3, dtype=int8)
    y: i8[3] = empty(3, dtype=int8)
    z: i8[3] = empty(3, dtype=int8)
    x[0] = i8(1)
    x[1] = i8(2)
    x[2] = i8(3)
    y[0] = i8(2)
    y[1] = i8(3)
    y[2] = i8(4)
    z = add_i8(size(x), x, y)
    assert z[0] == i8(3)
    assert z[1] == i8(5)
    assert z[2] == i8(7)
    z = add_i8_loop(size(x), x, y)
    assert z[0] == i8(3)
    assert z[1] == i8(5)
    assert z[2] == i8(7)

def main_u8():
    x: u8[3] = empty(3, dtype=uint8)
    y: u8[3] = empty(3, dtype=uint8)
    z: u8[3] = empty(3, dtype=uint8)
    x[0] = u8(1)
    x[1] = u8(2)
    x[2] = u8(3)
    y[0] = u8(2)
    y[1] = u8(3)
    y[2] = u8(4)
    z = add_u8(size(x), x, y)
    assert z[0] == u8(3)
    assert z[1] == u8(5)
    assert z[2] == u8(7)
    z = add_u8_loop(size(x), x, y)
    assert z[0] == u8(3)
    assert z[1] == u8(5)
    assert z[2] == u8(7)

def main_u16():
    x: u16[3] = empty(3, dtype=uint16)
    y: u16[3] = empty(3, dtype=uint16)
    z: u16[3] = empty(3, dtype=uint16)
    x[0] = u16(1)
    x[1] = u16(2)
    x[2] = u16(3)
    y[0] = u16(2)
    y[1] = u16(3)
    y[2] = u16(4)
    z = add_u16(size(x), x, y)
    assert z[0] == u16(3)
    assert z[1] == u16(5)
    assert z[2] == u16(7)
    z = add_u16_loop(size(x), x, y)
    assert z[0] == u16(3)
    assert z[1] == u16(5)
    assert z[2] == u16(7)

def main_u32():
    x: u32[3] = empty(3, dtype=uint32)
    y: u32[3] = empty(3, dtype=uint32)
    z: u32[3] = empty(3, dtype=uint32)
    x[0] = u32(1)
    x[1] = u32(2)
    x[2] = u32(3)
    y[0] = u32(2)
    y[1] = u32(3)
    y[2] = u32(4)
    z = add_u32(size(x), x, y)
    assert z[0] == u32(3)
    assert z[1] == u32(5)
    assert z[2] == u32(7)
    z = add_u32_loop(size(x), x, y)
    assert z[0] == u32(3)
    assert z[1] == u32(5)
    assert z[2] == u32(7)

def main_u64():
    x: u64[3] = empty(3, dtype=uint64)
    y: u64[3] = empty(3, dtype=uint64)
    z: u64[3] = empty(3, dtype=uint64)
    x[0] = u64(1)
    x[1] = u64(2)
    x[2] = u64(3)
    y[0] = u64(2)
    y[1] = u64(3)
    y[2] = u64(4)
    z = add_u64(size(x), x, y)
    assert z[0] == u64(3)
    assert z[1] == u64(5)
    assert z[2] == u64(7)
    z = add_u64_loop(size(x), x, y)
    assert z[0] == u64(3)
    assert z[1] == u64(5)
    assert z[2] == u64(7)

main_i8()
main_u8()
main_u16()
main_u32()
main_u64()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()
