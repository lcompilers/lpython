from lpython import i32, f64

def test_tuple_concat():
    t1: tuple[i32, i32]
    t2: tuple[f64, str, i32]
    t3: tuple[f64]
    t4: tuple[i32, i32, f64, str, i32, f64]
    t5: tuple[i32, f64]
    t6: tuple[i32, f64, i32, f64]
    t7: tuple[i32, f64, i32, f64, i32, f64]
    t8: tuple[i32, f64, i32, f64, i32, f64, i32, f64]
    l1: list[tuple[i32, f64]] = []
    start: i32
    i:i32
    t9: tuple[tuple[i32, i32], tuple[f64, str, i32]]

    t1 = (1, 2)
    t2 = (3.0, "abc", -10)
    t3 = (10.0,)
    t4 = t1 + t2 + t3
    assert t4 == (t1[0], t1[1], t2[0], t2[1], t2[2], t3[0])
    assert t4 + t3 == t1 + t2 + t3 + (t3[0],)

    start = 117
    for i in range(start, start + 3):
        t5 = (i, f64(i*i))
        l1.append(t5)
        if i == start:
            t6 = t5 + l1[-1]
        elif i == start + 1:
            t7 = t6 + l1[-1]
        else:
            t8 = t7 + l1[-1]
    
    assert t6 == l1[0] + l1[0]
    assert t7 == t6 + l1[1]
    assert t8 == t7 + l1[2]

    t9 = (t1,) + (t2,)
    assert t9[0] == t1 and t9[1] == t2

test_tuple_concat()