from lpython import i32, f64

def test_tuple_nested():
    t1: tuple[i32, i32] = (-1, -2)
    t2: tuple[i32, i32] = (-3, -4)
    t3: tuple[tuple[i32, i32], tuple[i32, i32], tuple[i32, i32],  tuple[i32, i32]]
    t4: tuple[i32, f64, str]
    t5: tuple[tuple[i32, f64, str], i32]
    t6: tuple[tuple[tuple[i32, f64, str], i32], f64]
    t7: tuple[tuple[i32, i32], tuple[list[i32], list[str]]]
    l1: list[tuple[i32, f64, tuple[f64, i32]]] = []
    i: i32
    s: str

    t3 = (t1, t2, t1, t2)
    assert t3[0] == t1 and t3[1] == t2
    assert t3[2] == t1 and t3[3] == t2

    t4 = (1, 2.0, 'abc')
    t5 = (t4, 3)
    assert t5 == ((1, 2.0, 'abc'), 3)
    t6 = (t5, 4.0)
    assert t6 == ((t4, 3), 4.0)

    for i in range(5):
        l1.append((i, f64(i+1), (f64(i+2), i+3)))
    for i in range(5):
        assert l1[i] == (i, f64(i+1), (f64(i+2), i+3))

    i = 3
    s = 'a'
    t7 = (t1, ([i, i+1, i+2], [s, s * 2, s * 3]))
    assert t7 == ((-1, -2), ([3, 4, 5], ['a', 'aa', 'aaa']))

test_tuple_nested()