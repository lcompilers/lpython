from lpython import i32, f64, str, dict, Const


def test_dict_const():
    d_int: Const[dict[str, i32]] = {"a": 1, "b": 2, "c": 3}

    assert d_int.get("a") == 1
    assert d_int.keys() == ["c", "a", "b"]
    assert d_int.values() == [3, 1, 2]

    d_float: Const[dict[str, f64]] = {"a": 1.0, "b": 2.0, "c": 3.0}
    
    assert d_float.get("a") == 1.0
    assert d_float.keys() == ["c", "a", "b"]
    assert d_float.values() == [3.0, 1.0, 2.0]


test_dict_const()
