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

if type(x) is int:
    pass

if ((2 + 3)/2 - 1 is 5) is True:
    pass

if x is not type(float):
    pass

if x is \
    not type(int):
    pass

a = [1, 2, 3]

if a not in [1, 2]:
    pass

if (a not in [1, 2]) not \
    in [True, False]:
    pass

if field in ["vararg", "kwarg"]:
    if value is None:
        pass

if (a in list1):
    pass

"hello" in x
'a' in a.func()
'lo' in 'hello'

for item in list1:
    if item in list2:
        pass

if a in list1 and b not in list2 or c in list3:
    pass

comp = [i**2 for i in range(10) if i not in [3, 5, 7] and i in list3]

# Fow showing the parsing order of 'in' in for-loop and 'in' in expr is correct
# and there is no conflict. Otherwise, the code below is gibberish.
for i in a in list1:
    pass

i: i32 = 4
if 2 > i : pass
if i > 5 : break
if i == 5 and i < 10 : i = 3

for i in range(N): # type: parallel
    c[i] = a[i] + scalar * b[i]

x = (y := 0)
if a := ord('3'):
    x = 1

a = {1, 2, 3}

# Ternary operator (Conditional expressions)

b = 6 if a == 2 else 8

'true' if True else 'false'

result = x if not (a > b) else y

print(a,"is greater") if (a > b) else print(b,"is Greater")
