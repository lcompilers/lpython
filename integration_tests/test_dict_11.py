from lpython import i32

def test_dict_11():
    num : dict[i32, i32]
    num = {11: 22, 33: 44, 55: 66}
    assert num.get(7, -1) == -1
    assert num.get(11, -1) == 22
    assert num.get(33, -1) == 44
    assert num.get(55, -1) == 66
    assert num.get(72, -110) == -110
    d : dict[i32, str]
    d = {1: "1", 2: "22", 3: "333"}
    assert d.get(2, "00") == "22"
    assert d.get(21, "nokey") == "nokey"

test_dict_11()
