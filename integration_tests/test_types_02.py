from lpython import i32, f64

def main0():
    a: f64
    a = 3.25
    b: i32 = i32(a) * 4
    print(b)
    assert b == 12

main0()
