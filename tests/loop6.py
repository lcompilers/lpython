def no_break_for():
    i: i32
    for i in range(2):
        print(i)
    else:
        print(10)

def no_break_while():
    i: i32 = 0
    while i < 2:
        print(i)
        i += 1
    else:
        print(10)

no_break_for()
no_break_while()
