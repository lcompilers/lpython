from lpython import i32, f64

def test_dict_keys_values():
    d1: dict[i32, i32] = {}
    k1: list[i32]
    k1_copy: list[i32] = []
    v1: list[i32]
    v1_copy: list[i32] = []
    i: i32
    j: i32
    key_count: i32

    for i in range(105, 115):
        d1[i] = i + 1
    k1 = d1.keys()
    for i in k1:
        k1_copy.append(i)
    v1 = d1.values()
    for i in v1:
        v1_copy.append(i)
    assert len(k1) == 10
    for i in range(105, 115):
        key_count = 0
        for j in range(len(k1)):
            if k1_copy[j] == i:
                key_count += 1
                assert v1_copy[j] == d1[i]
        assert key_count == 1

test_dict_keys_values()
