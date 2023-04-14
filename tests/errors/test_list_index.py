from lpython import i32

def test_list_index_error():
    a: list[i32]
    a = [1, 2, 3]
    # a.index(1.0)  # type mismatch
    print(a.index(0)) # no error?

test_list_index_error()
