def test(a: i32, b: i32) -> i32:
    return a**b

def check() -> i32:
    a: i32
    a = test(2, 2)
    return a
