from ltypes import i32

def test_list_01(n: i32) -> i32:
    a: list[i32] = []
    i: i32
    for i in range(n):
        a.append(i)
    sum: i32 = 0
    for i in range(n):
        sum += a[i]
    return sum

def test_list_insert_02(x: list[i32], n: i32) -> list[i32]:
    i: i32
    imod: i32
    for i in range(n):
        imod = i % 3
        if imod == 0:
            x.insert(0, i + n)
        elif imod == 1:
            x.insert(len(x), i + n + 1)
        elif imod == 2:
            x.insert(len(x)//2, i + n + 2)

    return x

def test_list_insert_03():
    l1:list[str] = ["a", "b", "c", "d"]
    l2:list[str] = []
    l1.insert(5,"e")
    s:str = "a"
    i:i32 = 0
    for i in range(len(l1)):
        assert l1[i] == chr(ord(s)+i)

def test_list_insert_04():
    l:list[i32] = [1, 2, 3, 4, 5]
    i:i32
    l.insert(6,6)
    l.insert(100,7)
    for i in range(1,8):
        assert l[i-1] == i

def test_list_02(n: i32) -> i32:
    x: list[i32] = [50, 1]
    acc: i32 = 0
    i: i32

    x = test_list_insert_02(x, n)

    for i in range(n):
        acc += x[i]
    return acc

def test_list_02_string():
    x: list[str] = []
    y: list[str] = []
    string: str
    i: i32

    for i in range(50):
        string = "xd_" + str(i + i % 3)
        y.append(string)

    imod: i32

    for i in range(50):
        imod = i % 3
        string  = "xd_" + str(i + imod)
        x.insert(len(x), string)

    for i in range(50):
        assert x[i] == y[i]


def verify():
    assert test_list_01(11) == 55
    assert test_list_02(50) == 3628
    test_list_02_string()
    test_list_insert_03()
    test_list_insert_04()

verify()
