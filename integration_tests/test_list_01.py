from ltypes import i32

def test_list_i32():
    a: list[i32]
    i: i32

    for i in range(10):
        a.append(i)

    for i in range(i):
        assert a[i] == i

test_list_i32()
