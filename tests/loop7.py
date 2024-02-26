def break_in_if_for():
    i: i32
    for i in range(2):
        print(i)
        if i == 1:
            break
    else:
        print(10)

def break_in_if_while():
    i: i32 = 0
    while i < 2:
        print(i)
        if i == 1:
            break
        i += 1
    else:
        print(10)

break_in_if_for()
