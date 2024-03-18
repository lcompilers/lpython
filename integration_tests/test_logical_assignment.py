from lpython import i32, f64


def test_logical_assignment():
    # Can be uncommented after fixing the segfault
    # _LPYTHON: str = "LPython"
    # s_var: str = "" or _LPYTHON
    # assert s_var == "LPython"
    # print(s_var)

    _MAX_VAL: i32 = 100
    i_var: i32 = 0 and 100
    assert i_var == 0
    print(i_var)

    _PI: f64 = 3.14
    f_var: f64 = 2.0 * _PI or _PI**2.0
    assert f_var == 6.28
    print(f_var)


test_logical_assignment()
