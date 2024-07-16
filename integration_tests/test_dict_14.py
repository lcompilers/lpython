from lpython import i32

def takes_dict(a: dict[i32, i32]) -> dict[i32, i32]:
    return {1:1, 2:2}

def test_dict():
    d_i32: dict[i32, i32] = {5: 1, 5: 2}
    d_str: dict[str, i32] = {'a': 1, 'a': 2}
    l_str_1: list[str] = []
    l_str_2: list[str] = []
    l_i32_1: list[i32] = []
    l_i32_2: list[i32] = []
    i: i32
    s: str

    assert len(d_i32) == 1
    d_i32.pop(5)
    assert len(d_i32) == 0

    assert len(d_str) == 1
    d_str.pop('a')
    assert len(d_str) == 0

    d_str = {'a': 2, 'a': 2, 'b': 2, 'c': 3, 'a': 5}
    assert len(d_str) == 3
    d_str.pop('a')
    assert len(d_str) == 2
    d_str.pop('b')
    assert len(d_str) == 1

    d_str['a'] = 20
    assert len(d_str) == 2
    d_str.pop('c')
    assert len(d_str) == 1

    l_str_1 = d_str.keys()
    for s in l_str_1:
        l_str_2.append(s)
    assert l_str_2 == ['a']
    l_i32_1 = d_str.values()
    for i in l_i32_1:
        l_i32_2.append(i)
    assert l_i32_2 == [20]

    d_i32 = {5: 2, 5: 2, 6: 2, 7: 3, 5: 5}
    assert len(d_i32) == 3
    d_i32.pop(5)
    assert len(d_i32) == 2
    d_i32.pop(6)
    assert len(d_i32) == 1

    d_i32[6] = 30
    assert len(d_i32) == 2
    d_i32.pop(7)
    assert len(d_i32) == 1

    l_i32_1 = d_i32.keys()
    l_i32_2.clear()
    for i in l_i32_1:
        l_i32_2.append(i)
    assert l_i32_2 == [6]
    l_i32_1 = d_i32.values()
    l_i32_2.clear()
    for i in l_i32_1:
        l_i32_2.append(i)
    assert l_i32_2 == [30]

    w: dict[i32, i32] = takes_dict({1:1, 2:2})
    assert len(w)  == 2

test_dict()
