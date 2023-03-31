from lpython import i32

# GLOBAL Dictionary
x: dict[i32, i32]
x = {0 : 0,  1: 1000, 2: 2000, 3 : 3000}

x[4] = 4000
assert len(x) == 5
x[5] = 5000
assert x[2] == 2000
i: i32
for i in range(len(x)):
    assert x[i] == i * 1000

# Copy of Dictionary
tmp: dict[i32, i32]
tmp = x
tmp[6] = 6000
assert len(tmp) == 7
assert tmp[6] == 6000
assert tmp[1] == 1000
