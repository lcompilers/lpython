from lpython import i32

def test_builtin_type_set():
    st: set[i32] = {1, 2, 3, 4}

    res = str(type(st))
    print(res)
    assert res == "<class 'set'>"
    

test_builtin_type_set()
