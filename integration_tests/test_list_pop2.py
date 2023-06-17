from lpython import i32, f64, InOut

def pop_wrapper(l: InOut[list[tuple[i32, f64]]], idx: i32) -> tuple[i32, f64]:
    return l.pop(idx)

def test_list_pop():
    l1: list[tuple[i32, f64]]
    x: tuple[i32, f64]
    i: i32

    l1 = [(1, 1.0), (2, 4.0), (3, 6.0)]

    x = pop_wrapper(l1, 0)
    assert x == (1, 1.0)

    l1.append((4, 8.0))
    assert x == (1, 1.0)

    for i in range(len(l1)):
        assert l1[i] == (i + 2, 2.0 * f64(i + 2))

test_list_pop()