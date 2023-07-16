from lpython import i16, i32

def f():
    i: i32 = 5
    print(i16(i % 1023))

def u16(x: i16) -> i32:
    if x >= i16(0):
        return i32(x)
    else:
        return i32(x) + 65536

f()
print(u16(i16(10)), u16(i16(-10)))
assert(u16(i16(10)) == 10)
assert(u16(i16(-10)) == 65526)
