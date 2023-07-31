from lpython import i32

def main0():
    a: list[i32] = [3, 4, 5]
    i: i32
    for i in range(10):
        a.append(1)
        a.pop()

    print(a)
    assert a[-1] == 5
    assert len(a) == 3

main0()
