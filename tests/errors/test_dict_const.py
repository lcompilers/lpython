from lpython import i32, f64, str, dict, Const


def test_dict_const():
    d: Const[dict[str, i32]] = {"a": 1, "b": 2, "c": 3}
    print(d.pop("a"))
    i: i32 = d.pop("a")
