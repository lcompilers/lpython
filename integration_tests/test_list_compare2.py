from lpython import i32

x: list[i32] = [1, 2, 3, 4]
y: list[i32] = [5, 6, 7, 8]
z: list[i32] = [1, 2, 3, 4]

assert(x != y)
assert(x == z)