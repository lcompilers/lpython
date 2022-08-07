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
    for i in range(n):
        x.insert(1, i)

    sum: i32 = 0
    for i in range(n):
        sum += x[i]
    return sum

def verify():
    assert test_list_01(11) == 55
    assert test_list_02(50) == 1275

verify()
