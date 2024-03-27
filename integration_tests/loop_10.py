def with_break_for():
    i: i32
    for i in range(4):
        print(i)
        break
    else:
        assert False


def with_break_while():
    i: i32 = 0
    while i < 4:
        print(i)
        break
    else:
        print(10)
        assert False


def no_break_for():
    i: i32
    for i in range(2):
        print(i)
    else:
        print(10)
        return
    assert False

def break_in_if_for():
    i: i32
    for i in range(2):
        print(i)
        if i == 1:
            break
    else:
        print(10)
        assert False

def nested_loop_for_for():
    i: i32
    j: i32
    for i in range(2):
        print("outer: " + str(i))
        for j in range(10, 20):
            print("  inner: " + str(j))
            break
    else:
        print("no break in outer loop")
        return
    assert False

def nested_loop_for_while():
    i: i32
    j: i32 = 10
    for i in range(2):
        print("outer: " + str(i))
        while j < 20:
            print("  inner: " + str(j))
            break
    else:
        print("no break in outer loop")
        return
    assert False

def nested_loop_while_for():
    i: i32 = 0
    j: i32
    while i < 2:
        print("outer: " + str(i))
        i += 1
        for j in range(10, 20):
            print("  inner: " + str(j))
            break
    else:
        print("no break in outer loop")
        return
    assert False

def nested_loop_while_while():
    i: i32 = 0
    j: i32 = 10
    while i < 2:
        print("outer: " + str(i))
        i += 1
        while j < 20:
            print("  inner: " + str(j))
            break
    else:
        print("no break in outer loop")
        return
    assert False


with_break_for()
with_break_while()
no_break_for()
break_in_if_for()
nested_loop_for_for()
nested_loop_for_while()
nested_loop_while_for()
nested_loop_while_while()
