from ltypes import i32, f64

def check_list_of_tuples(l: list[tuple[i32, f64, str]], sign: i32):
    size: i32 = len(l)
    i: i32
    t: tuple[i32, f64, str]
    string: str

    for i in range(size):
        t = l[i]
        string = str(sign * i) + "_str"
        assert t[0] == sign * i
        assert l[i][0] == sign * i

        assert t[1] == float(sign * i)
        assert l[i][1] == float(sign * i)

        assert t[2] == string
        assert l[i][2] == string

def fill_list_of_tuples(size: i32) -> list[tuple[i32, f64, str]]:
    l1: list[tuple[i32, f64, str]] = []
    t: tuple[i32, f64, str]
    i: i32

    for i in range(size):
        t = (i, float(i), str(i) + "_str")
        l1.append(t)

    return l1

def insert_tuples_into_list(l: list[tuple[i32, f64, str]], size: i32) -> list[tuple[i32, f64, str]]:
    i: i32
    string: str
    t: tuple[i32, f64, str]

    for i in range(size//2, size):
        string  = str(i) + "_str"
        t = (i, float(i), string)
        l.insert(i, t)

    return l

def test_list_of_tuples():
    l1: list[tuple[i32, f64, str]] = []
    t: tuple[i32, f64, str]
    size: i32 = 20
    i: i32
    string: str

    l1 = fill_list_of_tuples(size)

    check_list_of_tuples(l1, 1)

    for i in range(size//2):
        l1.remove(l1[len(l1) - 1])

    t = l1[len(l1) - 1]
    assert t[0] == size//2 - 1
    assert t[1] == size//2 - 1
    assert t[2] == str(size//2 - 1) + "_str"

    l1 = insert_tuples_into_list(l1, size)

    check_list_of_tuples(l1, 1)

    for i in range(size):
        string  = str(-i) + "_str"
        t = (-i, float(-i), string)
        l1[i] = t

    check_list_of_tuples(l1, -1)

test_list_of_tuples()
