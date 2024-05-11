from lpython import i32

l: list[i32] = [1, 2, 3, 4]
print("Before Pop:", l)

assert len(l) == 4
assert l[0] == 1
assert l[1] == 2
assert l[2] == 3
assert l[3] == 4

x: i32 = l.pop()
print("After Pop:", l)

assert x == 4
assert len(l) == 3
assert l[0] == 1
assert l[1] == 2
assert l[2] == 3

print("Popped Element: ", x)
