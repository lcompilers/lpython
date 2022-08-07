from ltypes import i32

def test_list_01(n: i32) -> i32:
    a: list[i32] = []
    i: i32
    for i in range(n):
        a.append(i)
    sum: i32 = 0
    for i in range(n):
        sum += a[i]
    return sum

def test_list_02(n: i32) -> i32:
    x: list[i32] = [50, 1]

    i: i32
    imod: i32
    for i in range(n):
        imod = i % 3
        if imod == 0:
            x.insert(0, i + n)
        elif imod == 1:
            x.insert(len(x), i + n + 1)
        elif imod == 2:
            x.insert(len(x)//2, i + n + 2)

    acc: i32 = 0
    for i in range(n):
        acc += x[i]
    return acc

def verify():
    assert test_list_01(11) == 55
    assert test_list_02(50) == 3628

verify()
