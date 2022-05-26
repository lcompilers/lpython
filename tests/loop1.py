def test_factorial_1(x: i32) -> i32:
    if x < 0:
        return 0
    result: i32
    result = 1
    while x > 0:
        result = result * x
        x -= 1
    return result


def test_factorial_2(x: i32) -> i32:
    result: i32
    result = 1
    i: i32
    for i in range(1, x + 1):
        result = result * i
    return result


def test_factorial_3(x: i32) -> i64:
    if x < 0:
        return 0
    result: i64
    result = 1
    while x > 0:
        result = result * x
        x -= 1
    return result


def main0():
    i: i32
    i = test_factorial_1(4)
    i = test_factorial_2(4)
    j: i64
    j = test_factorial_3(5)
    print(i, j)

main0()
