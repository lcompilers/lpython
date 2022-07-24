def func():
    x: list[i32]
    x = [1, 2, 3]

    i: i32
    for i in x:
        yield i

    for i in x:
        yield (i)
