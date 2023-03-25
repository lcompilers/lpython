from lpython import i32

def fill_list_str(size: i32) -> list[str]:
    a: list[str] = ["0_str", "1_str"]
    i: i32
    for i in range(size):
        a.append(str(i + 2) + "_str")
    return a

def test_list_01():
    a: list[str] = []
    b: list[str]
    string: str = "string_"
    b = [string, string]
    i: i32

    a = fill_list_str(10)

    for i in range(10):
        b.append(string + str(i + 2))


    for i in range(12):
        print(a[i], b[i])

    a[11] = "no_str"
    a[10] = string

    for i in range(12):
        if i32(i / 2) == 2 * i:
            b[i] = str(i) + "_str"
        else:
            string  = str(i) + "_str"
            b[i] = string

    for i in range(10):
        assert b[i] == a[i]

def test_list_02():
    x: list[str] = ["1", "2"]
    y: list[str] = []

    i:i32
    for i in range(50):
        x.append("str_" + str(i))

    x[0] = "str_" + x[0]
    x[1] = "str_" + x[1]

    s: str
    for s in x:
        y.append(s)

    for i in range(len(y)):
        assert x[i] == y[i]


def tests():
    test_list_01()
    test_list_02()

tests()
