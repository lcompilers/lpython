from lpython import i32, f64

def test_dict_keys_values():
    d1: dict[i32, i32] = {}
    k1: list[i32]
    k1_copy: list[i32] = []
    v1: list[i32]
    v1_copy: list[i32] = []
    i: i32
    j: i32
    s: str
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

    d2: dict[str, str] = {}
    k2: list[str]
    k2_copy: list[str] = []
    v2: list[str]
    v2_copy: list[str] = []

    for i in range(105, 115):
        d2[str(i)] = str(i + 1)
    k2 = d2.keys()
    for s in k2:
        k2_copy.append(s)
    v2 = d2.values()
    for s in v2:
        v2_copy.append(s)
    assert len(k2) == 10
    for i in range(105, 115):
        key_count = 0
        for j in range(len(k2)):
            if k2_copy[j] == str(i):
                key_count += 1
                assert v2_copy[j] == d2[str(i)]
        assert key_count == 1
    
    # dict.values on dict constant
    assert len({1: "a"}.values()) == 1
    print({1: "a"}.values())

    assert len({"a": 1, "b": 2, "c": 3}.values()) == 3
    print({"a": 1, "b": 2, "c": 3}.values())

    assert len({1: [1, 2, 3], 2: [4, 5, 6], 3: [7, 8, 9]}.values()) == 3
    print({1: [1, 2, 3], 2: [4, 5, 6], 3: [7, 8, 9]}.values())

    assert len({(1, 2): "a", (3, 4): "b", (5, 6): "c"}.values()) == 3
    print({(1, 2): "a", (3, 4): "b", (5, 6): "c"}.values())

    k_1: list[list[i32]] = {"list1": [1, 2, 3], "list2": [4, 5, 6], "list3": [7, 8, 9]}.values()
    assert len(k_1) == 3
    print(k_1)

    k_2: list[str] = {(1, 2): "a", (3, 4): "b", (5, 6): "c"}.values()
    assert len(k_2) == 3
print(k_2)


test_dict_keys_values()
