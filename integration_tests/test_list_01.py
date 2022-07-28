from ltypes import i32

def test_list_i32():
    a: list[i32] = [0, 1, 2, 3, 4]
    i: i32

    for i in range(10):
        a.append(i + 5)

    for i in range(15):
        assert a[i] == i

test_list_i32()
