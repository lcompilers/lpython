from lpython import i32, f64

def test_dict_keys_values():
    d1: dict[i32, i32] = {}
    d2: dict[tuple[i32, i32], tuple[i32, tuple[str, f64]]] = {}
    k1: list[i32]
    k2: list[tuple[i32, i32]]
    v1: list[i32]
    v2: list[tuple[i32, tuple[str, f64]]]
    i: i32
    j: i32
    key_count: i32
    s: str

    for i in range(105, 115):
        d1[i] = i + 1
    k1 = d1.keys()
    v1 = d1.values()
    assert len(k1) == 10
    for i in range(105, 115):
        key_count = 0
        for j in range(len(k1)):
            if k1[j] == i:
                key_count += 1
                assert v1[j] == d1[i]
        assert key_count == 1

    s = 'a'
    for i in range(10):
        d2[(i, i + 1)] = (i, (s, f64(i * i)))
        s += 'a'
    k2 = d2.keys()
    v2 = d2.values()
    assert len(k2) == 10
    for i in range(10):
        key_count = 0
        for j in range(len(k2)):
            if k2[j] == (i, i + 1):
                key_count += 1
                assert v2[j] == d2[k2[j]]
        assert key_count == 1

test_dict_keys_values()