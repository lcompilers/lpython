from lpython import u16, i32, u8, u32, u64

# test issue 2170

u_1 : u16 = u16(32768)
u_2 : u8 = u8(24)
u_3 : u32 = u32(32768)
u_4 : u64 = u64(32768)

assert u_1 == u16(32768)
assert u_2 == u8(24)
assert u_3 == u32(32768)
assert u_4 == u64(32768)

print(u_1, u_2, u_3, u_4)
