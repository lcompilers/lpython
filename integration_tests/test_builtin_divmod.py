from lpython import i32

def f():
    i: i32 = 42356
    j: i32 = -525
    dm: tuple[i32, i32] = divmod(i, j)
    assert dm[0] == -81
    assert dm[1] == -169
    (i, j) = divmod(-42, -526)
    assert i == 0
    assert j == -42

f()
