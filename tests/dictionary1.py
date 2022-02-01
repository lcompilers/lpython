def test_Dict():
    x: i32
    y: i32

    # x = {1: "2", "3": 4} -> sematic error
    x = {1: 2, 3: 4}
    x = {"a": -1, "b": -2}

    y = x["a"]
    y = x["b"]
