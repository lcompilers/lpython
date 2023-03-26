from lpython import i32

def test_oct():
    i: i32
    i = 34
    assert oct(i) == "0o42"
    i = -4235
    assert oct(i) == "-0o10213"
    assert oct(34) == "0o42"
    assert oct(-4235) == "-0o10213"

test_oct()