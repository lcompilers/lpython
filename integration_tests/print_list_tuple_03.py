from lpython import i32

def f():
    # issue 1862
    x: dict[i32, tuple[i32, i32]]
    x = {1: (1, 2), 2: (3, 4)}
    y: dict[i32, list[i32]]
    y = {1: [1, 2], 2: [3, 4]}
    print(x[1], x[2], y[1])

f()
