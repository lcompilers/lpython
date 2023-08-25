from lpython import i32

def test_dict_pop():
    d: dict[str, i32] = {'a': 2}
    d.pop('a')
    d.pop('a')

test_dict_pop()
