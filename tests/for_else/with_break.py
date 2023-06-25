def with_break():
    i: i32
    for i in range(4):
        print(i)
        break
    else:
        print(10)

with_break()
