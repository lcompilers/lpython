from ltypes import i32, f64, dataclass

@dataclass
class A:
    x: i32
    y: f64

def f(x_: i32, y_: f64) -> A:
    a_struct: A = A(x_, y_)
    return a_struct

def test_struct_return():
    b: A = f(0, 1.0)
    print(b.x, b.y)
    assert b.x == 0
    assert b.y == 1.0

test_struct_return()
