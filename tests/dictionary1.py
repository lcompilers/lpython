def test_Dict():
    x: dict[i32, i32]
    x = {1: 2, 3: 4}
    # x = {1: "2", "3": 4} -> sematic error

    y: dict[str, i32]
    y = {"a": -1, "b": -2}

    z: i32
    # this assignment should eventually work
    z = x["a"]
    z = x["b"]
