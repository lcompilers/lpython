from lpython import i32, f64, reserve

def test_list_reserve():
    l1: list[i32] = []
    l2: list[list[tuple[f64, str, tuple[i32, f64]]]] = []
    i: i32

    reserve(l1, 100)
    # for i in range(50):
    #     l1.append(i)
    #     assert len(l1) == i + 1

    # reserve(l1, 150)

    # for i in range(50):
    #     l1.pop(0)
    #     assert len(l1) == 49 - i

    # reserve(l2, 100)
    # for i in range(50):
    #     l2.append([(f64(i * i), str(i), (i, f64(i + 1))), (f64(i), str(i), (i, f64(i)))])
    #     assert len(l2) == i + 1

    # reserve(l2, 150)

    # for i in range(50):
    #     l2.pop(0)
    #     assert len(l2) == 49 - i

test_list_reserve()
