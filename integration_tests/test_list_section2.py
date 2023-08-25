from lpython import i32


def test_list_section():
    x: list[i32]
    x = [5, -6, 7, -1, 2, 10, -8, 15]

    n: i32 = len(x[1:4])
    print(n)
    assert n == 3

test_list_section()
