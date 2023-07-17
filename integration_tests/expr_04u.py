from lpython import u8, u16, u32, u64

FLAG1 : u8 = u8(1) << u8(4)
FLAG2 : u16 = u16(1) << u16(4)
FLAG3: u32 = u32(1) << u32(4)
FLAG4: u64 = u64(1) << u64(4)

print(FLAG1, FLAG2, FLAG3, FLAG4)
assert FLAG1 == u8(16)
assert FLAG2 == u16(16)
assert FLAG3 == u32(16)
assert FLAG4 == u64(16)
