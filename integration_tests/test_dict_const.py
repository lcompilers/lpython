from lpython import i32, f64, str, dict, Const


def test_dict_const():
    CONST_DICTIONARY_INTEGR: Const[dict[str, i32]] = {"a": 1, "b": 2, "c": 3}

    assert CONST_DICTIONARY_INTEGR.get("a") == 1
    assert CONST_DICTIONARY_INTEGR.keys() == ["c", "a", "b"]
    assert CONST_DICTIONARY_INTEGR.values() == [3, 1, 2]

    CONST_DICTIONARY_FLOAT: Const[dict[str, f64]] = {"a": 1.0, "b": 2.0, "c": 3.0}
    
    assert CONST_DICTIONARY_FLOAT.get("a") == 1.0
    assert CONST_DICTIONARY_FLOAT.keys() == ["c", "a", "b"]
    assert CONST_DICTIONARY_FLOAT.values() == [3.0, 1.0, 2.0]


test_dict_const()
