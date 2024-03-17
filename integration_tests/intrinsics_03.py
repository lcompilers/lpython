from lpython import i32

a : i32
b : i32

a = 5
b = 2
print(lshift(a,b))
assert(lshift(a,b) == a<<b)

print(lshift(5,2))
assert(lshift(5,2) == 5<<2)