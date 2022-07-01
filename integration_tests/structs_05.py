from ltypes import i32, f64, dataclass

@dataclass
class A:
    y: f64
    x: i32

def g():
    y: A[2]
    eps: f64 = 1e-12
    y[0].x = 1
    y[0].y = 1.1
    y[1].x = 2
    y[1].y = 2.2
    print(y[0].x, y[0].y)
    assert y[0].x == 1
    assert abs(y[0].y - 1.1) < eps
    print(y[1].x, y[1].y)
    assert y[1].x == 2
    assert abs(y[1].y - 2.2) < eps

g()
