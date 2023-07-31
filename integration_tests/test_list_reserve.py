from lpython import i32, f64

def test_list_reserve():
    l1: list[i32] = []
    l2: list[list[tuple[f64, str, tuple[i32, f64]]]] = []
    i: i32

    l1.reserve(100)
    for i in range(50):
        l1.append(i)
        assert len(l1) == i + 1

    l1.reserve(150)

    for i in range(50):
        l1.pop(0)
        assert len(l1) == 49 - i

    l2.reserve(100)
    for i in range(50):
        l2.append([(f64(i * i), str(i), (i, f64(i + 1))), (f64(i), str(i), (i, f64(i)))])
        assert len(l2) == i + 1

    l2.reserve(150)

    for i in range(50):
        l2.pop(0)
        assert len(l2) == 49 - i

test_list_reserve()
