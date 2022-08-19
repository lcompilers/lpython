from ltypes import i32, f64

def sort_list():
    x: list[i32]
    x = []
    size: i32 = 50
    i: i32; j: i32

    for i in range(size):
        x.append(size - i)

    for i in range(size):
        for j in range(i + 1, size):
            if x[i] > x[j]:
                x[i], x[j] = x[j], x[i]

    for i in range(size - 1):
        assert x[i] <= x[i + 1]

    assert len(x) == size

sort_list()
