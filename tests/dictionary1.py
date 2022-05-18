def test_Dict():
    x: dict[i32, i32]
    x = {1: 2, 3: 4}
    # x = {1: "2", "3": 4} -> sematic error

    y: dict[str, i32]
    y = {"a": -1, "b": -2}

    z: i32
    z = y["a"]
    z = y["b"]
    z = x[1]


def test_dict_insert():
    y: dict[str, i32]
    y = {"a": -1, "b": -2}
    y["c"] = -3


def test_dict_get():
    y: dict[str, i32]
    y = {"a": -1, "b": -2}
    x: i32
    x = y.get("a")
    x = y.get("a", 0)


def test_dict_pop():
    y: dict[str, i32]
    y = {"a": 1, "b": 2}
    x: i32
    x = y.pop("a")
