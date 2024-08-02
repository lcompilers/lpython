def test_int_dict():
    a: dict[i32, i32] = {1:2, 2:3, 3:4, 4:5}
    i: i32
    assert (1 in a)
    assert (6 not in a)
    i = 4
    assert (i in a)

def test_str_dict():
    a: dict[str, str] = {'a':'1', 'b':'2', 'c':'3'}
    i: str
    assert ('a' in a)
    assert ('d' not in a)
    i = 'c'
    assert (i in a)

def test_int_set():
    a: set[i32] = {1, 2, 3, 4}
    i: i32
    assert (1 in a)
    assert (6 not in a)
    i = 4
    assert (i in a)

def test_str_set():
    a: set[str] = {'a', 'b', 'c', 'e', 'f'}
    i: str
    assert ('a' in a)
    assert ('d' not in a)
    i = 'c'
    assert (i in a)

test_int_dict()
test_str_dict()
test_int_set()
test_str_set()
