def break_in_if():
    i: i32
    for i in range(4):
        print(i)
        if i == 2:
            break
    else:
        print(10)

break_in_if()
