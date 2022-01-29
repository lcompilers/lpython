def main() -> i32:
    x: i32
    x = 5
    result: i32
    result = 1
    while x > 0:
        result = result * x
        x -= 1
    print(result)
    return 0
