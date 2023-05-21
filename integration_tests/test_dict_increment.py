from lpython import i32, f64

def test_dict_increment():
    d_int_int: dict[i32, i32]
    d_int_float: dict[i32, f64]
    d_str_int: dict[str, i32]
    d_int_str: dict[i32, str]
    i1: i32
    i2: i32
    j1: f64
    s1: str

    d_int_int = {0: 1}
    d_int_int[0] += 1000
    assert d_int_int[0] == 1001

    i2 = 1
    d_int_int = {1: i2}
    for i1 in range(10):
        d_int_int[1] += i1
        i2 += i1
        assert d_int_int[1] == i2
    
    j1 = 2.0
    d_int_float = {2: j1}
    while j1 < 4.0:
        d_int_float[2] += 0.1
        j1 += 0.1
        assert d_int_float[2] == j1

    # Not working with str key. Example:

    # d_str_int = {"1": 1}
    # print(d_str_int["1"])
    # d_str_int["1"] += 1
    # print(d_str_int["1"])
    # d_str_int["1"] = d_str_int["1"] + 1
    # print(d_str_int["1"])

    # j1 = 2.0
    # d_str_int = {'key': j1}
    # while j1 < 4.0:
    #     d_str_int['key'] += 0.1
    #     j1 += 0.1
    #     print(d_str_int['key'], j1)
    #     assert d_str_int['key'] == j1

    s1 = "0"
    d_int_str = {-1: s1}
    for i1 in range(10):
        d_int_str[-1] += str(i1)
        s1 += str(i1)
        assert d_int_str[-1] == s1

test_dict_increment()