from lpython import i32

def test_dict_pop():
    d: dict[i32, i32] = {1: 2}
    d.pop(1)
    d.pop(1)

test_dict_pop()
