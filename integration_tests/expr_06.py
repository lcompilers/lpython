from ltypes import i32, f32, f64
from numpy import empty, cos, sin

def main0():
    x: i32 = 25
    y: i32 = (2 + 3) * 5
    z: f32 = (2.0 + 3) * 5.0
    xa: i32[3] = empty(3)
    assert x == 25
    assert y == 25
    assert z == 25.0

def test_issue_892():
    i: i32
    for i in range(1, 1000):
        x: i32; p1: f64; p2: f64; y: f64
        x = i
        y = float(x)
        p1 = 2*sin(y)*cos(y)
        p2 = sin(2*y)
        assert abs(p1 - p2) <= 1e-12

    for i in range(1000, 1000 + 100 , 2):
        x: i32; p1: f64; p2: f64; y: f64
        x = i
        y = float(x)
        p1 = sin(y)**2
        p2 = cos(y)**2
        assert abs(p1 + p2 - 1.0) <= 1e-12

main0()
test_issue_892()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()
