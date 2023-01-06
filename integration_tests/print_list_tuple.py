from ltypes import f64, i32


def test_print_list():
    a: list[str] = ["ab", "abc", "abcd"]
    b: list[i32] = [1, 2, 3, 4]
    c: list[f64] = [1.23 , 324.3 , 56.431, 90.5, 34.1]
    d: list[i32] = []

    print(a, b)
    print(c)
    print(d)

    x: tuple[i32, str, i32] = (12, 'lpython', 32)
    y: tuple[i32, str, str, i32] = (1, 'test', 'tuple', 3)
    z: tuple[f64, f64, i32, bool] = (4.3, 6.45, 8, True)

    print(x, y, z)


test_print_list()
