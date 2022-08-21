def func():
    x: list[i32]
    x = [1, 2, 3]

    i: i32
    for i in x:
        yield i

    for i in x:
        yield (i)

    for i in x:
        yield ()

    for _ in x:
        yield

    yield x, y
    yield x, y,

def iterable1():
    yield 1
    yield 2

def iterable2():
    yield from iterable1()
    yield 3

assert list(iterable2()) == [1, 2, 3]
