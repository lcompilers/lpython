def test_list_const():
    CONST_INTEGER_LIST: Const[list[i32]] = [1, 2, 3, 4, 5, 1]

    CONST_INTEGER_LIST.append(6)
    CONST_INTEGER_LIST.insert(3, 8)
    CONST_INTEGER_LIST.reverse()
    CONST_INTEGER_LIST.pop()
    CONST_INTEGER_LIST.pop(0)
    CONST_INTEGER_LIST.clear()
    CONST_INTEGER_LIST.remove(1)


