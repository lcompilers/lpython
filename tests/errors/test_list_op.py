from lpython import i32

def test_list_op():
    a: list[i32]
    a = [1, 2, 3]
    b: list[i32]
    b = [4,5]
    c:list[i32] = a-b

test_list_op()