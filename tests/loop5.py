def with_break_for():
    i: i32
    for i in range(4):
        print(i)
        break
    else:
        print(10)

def with_break_while():
    i: i32 = 0
    while i < 4:
        print(i)
        break
    else:
        print(10)

with_break_for()
with_break_while()
