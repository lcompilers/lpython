from ltypes import i32, f64, dataclass

@dataclass
class A:
    y: f64
    x: i32

def verify(s: A[:], x1: i32, y1: f64, x2: i32, y2: f64):
    eps: f64 = 1e-12
    s0: A = s[0]
    print(s0.x, s0.y)
    assert s0.x == x1
    assert abs(s0.y - y1) < eps

    s1: A = s[1]
    print(s1.x, s1.y)
    assert s1.x == x2
    assert abs(s1.y - y2) < eps

def update_1(s: A):
    s.x = 2
    s.y = 1.2

def update_2(s: A[:]):
    s[1].x = 3
    s[1].y = 2.3

def g():
    # TODO: Replace y: A[2] with y: A[2] = [None, None]
    # TODO: And enable cpython in integration_tests.
    y: A[2]
    y[0] = A(1.1, 1)
    y[1] = A(2.2, 2)
    verify(y, 1, 1.1, 2, 2.2)
    update_1(y[0])
    update_2(y)
    verify(y, 2, 1.2, 3, 2.3)

g()
