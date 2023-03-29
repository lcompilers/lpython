from lpython import f64, c64, i32, c32

def test1() -> f64:
    x: f64
    x = 1.0
    return x


def test2() -> c64:
    x: c64
    x = complex(2, 2)
    return x


def test3() -> i32:
    x: c64
    x = 4j
    y: c32
    y = c32(4j)
    return 0


print(test1())
print(test2())
print(test3())
