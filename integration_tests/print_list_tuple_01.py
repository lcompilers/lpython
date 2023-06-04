from lpython import f64, i32, c64, InOut


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
    t: tuple[i32, f64, str, c64]
    t = (500, 240.02, 'complex-test', complex(42.0, 1.0))

    print(x, y, z, t)


def f(y: InOut[list[i32]]) -> list[i32]:
    y.append(4)
    return y

def g():
    print(f([1,2,3]))


def check():
    g()
    test_print_list()


check()
