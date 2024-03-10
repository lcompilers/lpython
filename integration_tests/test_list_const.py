from lpython import i32, i64, f32, f64, dict, list, tuple, str, Const, c64


def test_list_const():
    CONST_INTEGER_LIST: Const[list[i32]] = [1, 2, 3, 4, 5, 1]

    assert CONST_INTEGER_LIST.count(1) == 2
    assert CONST_INTEGER_LIST.index(1) == 0

    CONST_STRING_LIST: Const[list[str]] = ["ALPHA", "BETA", "RELEASE"]
    assert CONST_STRING_LIST.count("ALPHA") == 1
    assert CONST_STRING_LIST.index("RELEASE") == 2


test_list_const()
