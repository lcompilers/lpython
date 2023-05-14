from lpython import i32, f64

def test_dict_bool():
    d_int: dict[bool, i32] = {}
    d_float: dict[bool, f64] = {}
    d_str: dict[bool, str] = {}
    i: i32
    j: f64
    s: str = ""
    l_str: list[str] = ["a", "b", "c", "d"]

    for i in range(10):
        d_int[True] = i
        assert d_int[True] == i

    for i in range(10, 20):
        d_int[True] = i
        d_int[False] = i + 1
        assert d_int[True] == d_int[False] - 1
        assert d_int[True] == i
    
    d_int[True] = 0
    d_int[False] = d_int[True]

    for i in range(10, 99):
        d_int[i%2 == 0] = d_int[i%2 == 0] + 1
    assert d_int[True] == d_int[False] + 1
    assert d_int[True] == 45

    j = 0.0
    while j < 1.0:
        d_float[False] = j + 1.0
        d_float[True] = d_float[False] * d_float[False]
        assert d_float[True] == (j + 1.0) * (j + 1.0)
        assert d_float[False] == j + 1.0
        j = j + 0.1

    d_str[False] = s

    for i in range(len(l_str)):
        d_str[True] = d_str[False]
        s += l_str[i]
        d_str[False] = s
        assert d_str[True] + l_str[i] == d_str[False]
        assert d_str[False] == s

test_dict_bool()
