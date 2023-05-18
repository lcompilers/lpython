def test_return_1(a: i32) -> i32:
    x: i32
    x = 5
    return x


def test_return_2(a: i32) -> str:
    x: str
    x = 'test'
    return x


def test_return_3(a: i32) -> i32:
    return a


def main0():
    i: i32
    i = test_return_1(4)
    s: str
    s = test_return_2(4)
    i = test_return_3(4)

main0()
