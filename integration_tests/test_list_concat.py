from lpython import i32, f64

def test_list_concat():
    t1: list[i32]
    t1 = [2] + [3]
    print(t1)
    assert len(t1) == 2
    assert t1[0] == 2
    assert t1[1] == 3

    t2: list[f64]
    t2 = [3.14, -4.5] + [1.233, -0.012, 5555.50]
    print(t2)
    assert len(t2) == 5
    assert abs(t2[0] - 3.14) <= 1e-5
    assert abs(t2[-1] - 5555.50) <= 1e-5

test_list_concat()
