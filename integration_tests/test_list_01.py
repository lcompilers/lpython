from ltypes import i32

def test_list_i32():
    a: list[i32] = [1]
    a.append(2)
    a.append(3)
    a.append(4)
    a.append(5)
    print(a[1])
    assert a[1] == 2 or a[1] == 3

test_list_i32()
