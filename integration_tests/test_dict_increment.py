from lpython import i32, f64

def test_dict_increment():
    d_int_int: dict[i32, i32]
    d_int_float: dict[i32, f64]
    d_bool_float: dict[bool, f64]
    d_str_float: dict[str, f64]
    d_int_str: dict[i32, str]
    i1: i32
    i2: i32
    j1: f64
    j2: f64
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

    i2 = 10
    d_int_int = {0: 0, 1: 0}
    for i1 in range(i2):
        d_int_int[i1 % 2] += 1
    assert d_int_int[0] == d_int_int[1]
    assert d_int_int[0] == i2 // 2

    j1 = 2.0
    d_int_float = {2: j1}
    while j1 < 4.0:
        d_int_float[2] += 0.1
        j1 += 0.1
        assert d_int_float[2] == j1

    j1 = 0.0
    j2 = 0.0
    d_bool_float = {True: 0.0, False: 0.0}
    while j1 < 4.0:
        d_bool_float[j1 < 2.0] += 0.1
        if j1 < 2.0:
            j2 += 0.1
        j1 += 0.1
    assert d_bool_float[j1 < 2.0] == d_bool_float[j1 > 2.0]
    assert d_bool_float[True] == j2

    j1 = 2.0
    d_str_float = {'key': j1}
    s1 = "ke"
    while j1 < 4.0:
        d_str_float[s1 + 'y'] += 0.1
        j1 += 0.1
        assert d_str_float['key'] == j1

    s1 = "0"
    d_int_str = {-1: s1}
    for i1 in range(10):
        d_int_str[-1] += str(i1)
        s1 += str(i1)
        assert d_int_str[-1] == s1

test_dict_increment()
