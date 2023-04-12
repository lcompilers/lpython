from lpython import i32

def test_dict_11():
    num : dict[i32, i32]
    num = {11: 22, 33: 44, 55: 66}
    assert num.get(7, -1) == -1

test_dict_11()
