from lpython import i32, f64

def main0():
    a: i32 = 4
    b: i32 = 3
    c: i32 = 12
    assert a * b != c, "Error: 3 * 4 equals 12"

main0()
