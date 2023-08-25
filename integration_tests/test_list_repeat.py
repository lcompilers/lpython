from lpython import i32, f64

def test_list_repeat():
    l_int_1: list[i32] = [1, 2]
    l_int_2: list[i32] = []
    l_int_3: list[i32]
    l_tuple_1: list[tuple[f64, i32]] = [(1.0, 2), (2.0, 4), (3.0, 6)]
    l_tuple_2: list[tuple[f64, i32]] = []
    l_tuple_3: list[tuple[f64, i32]]
    l_str_1: list[str] = ['ab', 'cd']
    l_str_2: list[str] = []
    l_str_3: list[str]
    i: i32

    assert len(l_int_1 * 0) == 0
    assert l_int_1 * 1 == [1, 2]
    assert l_int_1 * 2 == [1, 2, 1, 2]

    for i in range(10):
        l_int_3 = l_int_1 * i
        assert l_int_3 == l_int_2
        l_int_2 += l_int_1

        l_tuple_3 = l_tuple_1 * i
        assert l_tuple_3 == l_tuple_2
        l_tuple_2 += l_tuple_1

        l_str_3 = l_str_1 * i
        assert l_str_3 == l_str_2
        l_str_2 += l_str_1

    for i in range(5):
        assert l_int_1 * i + l_int_1 * (i + 1) == l_int_1 * (2 * i + 1)
        assert l_tuple_1 * i + l_tuple_1 * (i + 1) == l_tuple_1 * (2 * i + 1)
        assert l_str_1 * i + l_str_1 * (i + 1) == l_str_1 * (2 * i + 1)

    print(l_int_1)
    print(l_tuple_1)
    print(l_tuple_1)

test_list_repeat()
