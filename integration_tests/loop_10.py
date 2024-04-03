def with_break_for():
    i: i32 = 0
    for i in range(4):
        i += 1
        break
    else:
        assert False


def with_break_while():
    i: i32 = 0
    while i < 4:
        i += 1
        break
    else:
        assert False


def no_break_for():
    i: i32
    j: i32 = 0
    for i in range(2):
        j += 1
    else:
        print(j)
        assert j == 2
        return
    assert False

def break_in_if_for():
    i: i32
    j: i32 = 0
    for i in range(2):
        j += 1
        if i == 1:
            break
    else:
        assert False
    print(j)
    assert j == 2

def nested_loop_for_for():
    i: i32
    j: i32
    m: i32 = 0
    for i in range(2):
        for j in range(10, 20):
            break
    else:
        m = 10
    print(m)
    assert m == 10

def nested_loop_for_while():
    i: i32
    j: i32 = 10
    m: i32 = 0
    for i in range(2):
        while j < 20:
            break
    else:
        m = 10
    print(m)
    assert m == 10

def nested_loop_while_for():
    i: i32 = 0
    j: i32
    m: i32 = 0
    while i < 2:
        i += 1
        for j in range(10, 20):
            break
    else:
        print(i)
        assert i == 2
        m = 10
    print(m)
    assert m == 10

def nested_loop_while_while():
    i: i32 = 0
    j: i32 = 10
    m: i32 = 0
    while i < 2:
        i += 1
        while j < 20:
            break
    else:
        print(i)
        assert i == 2
        m = 10
    print(m)
    assert m == 10

def nested_loop_else():
    i: i32
    j: i32
    l: i32 = 0
    m: i32 = 0
    for i in range(2):
        l += 1
        m: i32 = 0
        for j in range(10, 12):
            m += 1
        else:
            print(m)
            assert m == 2
            l += 10
    else:
        print(l)
        assert l == 22
        m = 10
    print(m)
    assert m == 10


with_break_for()
with_break_while()
no_break_for()
break_in_if_for()
nested_loop_for_for()
nested_loop_for_while()
nested_loop_while_for()
nested_loop_while_while()
nested_loop_else()
