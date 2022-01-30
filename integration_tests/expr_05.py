def test_multiply(a: i32, b: i32) -> i32:
    return a*b

def main() -> i32:
    a: i32
    b: i32
    a = 10
    b = -5
    print(test_multiply(a, b))
    return 0
