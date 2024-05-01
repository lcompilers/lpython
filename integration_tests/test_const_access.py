from lpython import i32, Const

CONST_LIST: Const[list[i32]] = [1, 2, 3, 4, 5]
CONST_DICTIONARY: Const[dict[str, i32]] = {"a": 1, "b": 2, "c": 3}

assert CONST_LIST[0] == 1
assert CONST_LIST[-2] == 4

assert CONST_DICTIONARY["a"] == 1