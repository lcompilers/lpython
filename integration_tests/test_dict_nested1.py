from lpython import i32

def test_nested_dict():
    d: dict[i32, dict[i32, i32]] = {1001: {2002: 3003}, 1002: {101: 2}}
    d[1001] = d[1002]
    d[1001][100] = 4005
    assert d[1001][100] == 4005

test_nested_dict()
