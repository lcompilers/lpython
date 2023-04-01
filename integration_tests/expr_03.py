from lpython import i32

def main0():
    x: i32
    x = 5
    result: i32
    result = 1
    while x > 0:
        result = result * x
        x -= 1
    assert result == 120

main0()
