from lpython import i32, f64, str, dict, Const


def test_dict_const():
    CONST_DICTIONARY: Const[dict[str, i32]] = {"a": 1, "b": 2, "c": 3}
    print(CONST_DICTIONARY.pop("a"))
    i: i32 = CONST_DICTIONARY.pop("a")

test_dict_const()
