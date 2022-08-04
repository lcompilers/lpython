from ltypes import i32

def test_list(n: i32) -> i32:
    a: list[i32] = []
    i: i32
    for i in range(n):
        a.append(i)
    sum: i32 = 0
    for i in range(n):
        sum += a[i]
    return sum

def verify():
    assert test_list(11) == 55

verify()
