from lpython import i32, Const


def test_const_list():
    CONST_INTEGER_LIST: Const[list[i32]] = [1, 2, 3, 4, 5, 1]

    print(CONST_INTEGER_LIST.count(1))
    print(CONST_INTEGER_LIST.index(1))
    assert CONST_INTEGER_LIST.count(1) == 2
    assert CONST_INTEGER_LIST.index(1) == 0

    CONST_STRING_LIST: Const[list[str]] = ["ALPHA", "BETA", "RELEASE"]
    print(CONST_STRING_LIST.count("ALPHA"))
    print(CONST_STRING_LIST.index("RELEASE"))
    assert CONST_STRING_LIST.count("ALPHA") == 1
    assert CONST_STRING_LIST.index("RELEASE") == 2


test_const_list()
