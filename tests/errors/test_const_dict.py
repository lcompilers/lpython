from lpython import i32, f64, str, dict, Const


def test_const_dict():
    CONST_DICTIONARY: Const[dict[str, i32]] = {"a": 1, "b": 2, "c": 3}
    print(CONST_DICTIONARY.pop("a"))


test_const_dict()
