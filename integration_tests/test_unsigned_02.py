from lpython import u16, i32

# test issue 2170

i : i32
u : u16 = u16(32768)
x : i32

for i in range(i32(u)):
    x = i * 2
