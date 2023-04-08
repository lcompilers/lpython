from lpython import i32

def test_list_count_error():
    a: list[i32]
    a = [1, 2, 3]
    a.count(1.0)