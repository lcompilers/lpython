from lpython import i32, list, Const


def test_const_list_reverse():
    CONST_INTEGER_LIST: Const[list[i32]] = [1, 2, 3, 4, 5, 1]

    CONST_INTEGER_LIST.reverse()


test_const_list_reverse()