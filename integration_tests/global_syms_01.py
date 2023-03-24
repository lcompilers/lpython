from lpython import i32

x: list[i32]
x = [1, 2]
i: i32
i = x[0]

def test_global_symbols():
    assert i == 1
    assert x[1] == 2

test_global_symbols()
