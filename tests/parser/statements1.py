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
x, y = 1, 2
x[i] = (1, 2)
x, y, z = t
(x, y, z) = t
x, = t
(x,) = t
obj = obj,

x += 1

x: i64
y: i32 = 1

del x
del ()
del (x, y)
del (x, y,)
del x, y,
del x, y

return
return a + b
return x(a)
return ()
return (x, y)
return (x, y, )
return x, y,
return x, y

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
32_768
1_2
234403_34_32_233_3
123.
123.45
12.34e+10
12+3j
.12+.001j
"String"
"String " "String"
'String ' 'String'
'String ' "String"
"String " "String"[1:]
x = ("String "
"String")
x = ("String " +
"String")
x = ("String " \
"String")
x = "String " \
"String"
x = "String "
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

"hello" in x
'a' in a.func()
'lo' in 'hello'

a.b[1]
a.b[1,]
a.b[1:]
a.b[:-1]
a.b[1:2]
a.b[:]
y.z[1:2:3]
y.z[1::3]
y.z[1::]

x = (y := 0)
if a := ord('3'):
    x = 1

a = {1, 2, 3}
a = {1, 2, 3,}

# Assigning outputs of expressions to variables

output: bool = x == y
output: bool
output = x == y

output: bool = x > y
output: bool
output = x > y

output: bool = [x] not in [y]
output: bool
output = [x] not in [y]

def comparison_return(a, b):
    return a > b

def comparison_return(a, b):
    return [a] in [b]

def comparison_return(a, b):
    output: bool
    output = [a] not in [b]
    return output

def comparison_return(a, b):
    output = [a] not in [b]
    return output
