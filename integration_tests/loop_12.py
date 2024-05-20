
def test_for_dict_int():
    dict_int: dict[i32, i32] = {1:2, 2:3, 3:4}
    key: i32
    s1: i32 = 0
    s2: i32 = 0

    for key in dict_int:
        print(key)
        s1 += key
        s2 += dict_int[key]

    assert s1 == 6
    assert s2 == 9

def test_for_dict_str():
    dict_str: dict[str, str] = {"a":"b", "c":"d"}
    key: str
    s1: str = ""
    s2: str = ""

    for key in dict_str:
        print(key)
        s1 += key
        s2 += dict_str[key]

    assert (s1 == "ac" or s1 == "ca")
    assert ((s1 == "ac" and s2 == "bd") or (s1 == "ca" and s2 == "db"))

def test_for_set_int():
    set_int: set[i32] = {1, 2, 3}
    el: i32
    s: i32 = 0

    for el in set_int:
        print(el)
        s += el

    assert s == 6

def test_for_set_str():
    set_str: set[str] = {'a', 'b'}
    el: str
    s: str = ""

    for el in set_str:
        print(el)
        s += el

    assert (s == "ab" or s == "ba")


