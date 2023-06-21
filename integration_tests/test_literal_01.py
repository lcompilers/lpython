from lpython import i32, Literal

def f():
    i_check : Literal[i32(0)] = 24
    assert i_check == 24
    bool_check: Literal[True]
    bool_check = False
    assert not bool_check
    str_check: Literal["lpython"] = "compiler"
    assert len(str_check) == 8
    assert str_check == "compiler"

f()
