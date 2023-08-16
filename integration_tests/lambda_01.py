from lpython import i32, Callable

def main0():
    x: Callable[[i32, i32, i32], i32] = lambda p, q, r: p +  q + r

    a123: i32 = x(1, 2, 3)
    a456: i32 = x(4, 5, 6)
    a_1_2_3: i32 = x(-1, -2, -3)

    print(a123, a456, a_1_2_3)

    assert a123 == 6
    assert a456 == 15
    assert a_1_2_3 == -6

main0()
