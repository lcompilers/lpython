from lpython import i32, f64, InOut

def insert_tuples_into_list(l: InOut[list[tuple[i32, f64, str]]], size: i32) -> list[tuple[i32, f64, str]]:
    i: i32
    string: str
    t: tuple[i32, f64, str]

    for i in range(size//2, size):
        string  = str(i) + "_str"
        t = (i, float(i), string)
        l.insert(i, t)

    return l

def fill_list_of_tuples(size: i32) -> list[tuple[i32, f64, str]]:
    l1: list[tuple[i32, f64, str]] = []
    t: tuple[i32, f64, str]
    i: i32

    for i in range(size):
        t = (i, float(i), str(i) + "_str")
        l1.append(t)

    return l1

def test_print_list_of_tuples():
    l1: list[tuple[i32, f64, str]] = []
    t: tuple[i32, f64, str]
    size: i32 = 20
    i: i32
    string: str

    l1 = fill_list_of_tuples(size)

    print(l1)

    for i in range(size//2):
        l1.remove(l1[len(l1) - 1])

    t = l1[len(l1) - 1]
    assert t[0] == size//2 - 1
    assert (t[1] - f64(size//2 - 1)) < 1e-10
    assert t[2] == str(size//2 - 1) + "_str"

    l1 = insert_tuples_into_list(l1, size)

    print(l1)

    for i in range(size):
        string  = str(-i) + "_str"
        t = (-i, float(-i), string)
        l1[i] = t

    print(l1)

def test_print_tuple():
    x: tuple[i32, str] = (3, "hiyo")
    y: tuple[str, str, str, i32, f64] = ("firstname", "middlename", "last name", -55, 3.14)
    print(x)
    print(y)

test_print_tuple()
test_print_list_of_tuples()
