from lpython import i32

def test_dict_key2():
    my_dict: dict[dict[i32, str], str] = {{1: "a", 2: "b"}: "first", {3: "c", 4: "d"}: "second"}

test_dict_key2()