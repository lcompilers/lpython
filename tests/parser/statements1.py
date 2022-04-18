# AST only tests

pass
break
continue

raise
raise NameError('String')
raise RuntimeError from exc

assert len(marks) != 0,"List is empty."
assert x == "String"

x = 1
x, y = x()
x = y = 1

x += 1

x: i64
y: i32 = 1

del x
del x, y

return
return a + b
return x(a)

global a
global a, b

nonlocal a
nonlocal a, b

# Expression
123
-123
-0x123
0x1ABC
-0o123
0O0127
-0b1101
0B1101
123.
123.45
12.34e+10
12+3j
.12+.001j
"String"
True
False

(x + y) * z
x - y
x * y
x / y
x % y
- y
+ y
~ y
x ** y
x // y
x @ y

x & y
x | y
x ^ y
x << y
x >> y

x == y
x != y
x < y
x <= y
x > y
x >= y
