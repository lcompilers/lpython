from lpython import i32

def test_slice_step_list():
    l: list[i32]
    l = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    print(l[0:10:0])

test_slice_step_list()