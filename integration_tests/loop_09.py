def with_break_for():
    i: i32
    s: i32 = 0
    for i in range(4):
        s += i
        break
    else:
        s += 10
    assert s == 0

def with_break_while():
    i: i32 = 0
    s: i32 = 0
    while i < 4:
        s += i
        break
    else:
        s += 10
    assert s == 0

def no_break_for():
    i: i32
    s: i32 = 0
    for i in range(2):
        s += i
    else:
        s += 10
    assert s == 11

def no_break_while():
    i: i32 = 0
    s: i32 = 0
    while i < 2:
        s += i
        i += 1
    else:
        s += 10
    assert s == 11

no_break_for()
no_break_while()

with_break_for()
with_break_while()

