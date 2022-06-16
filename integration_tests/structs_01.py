from ltypes import i32, f32, dataclass

@dataclass
class A:
    y: f32
    x: i32

def f(a: A):
    print(a.x)
    print(a.y)

def g():
    x: A
    x = A(3.25, 3)
    f(x)
    assert x.x == 3
    assert x.y == 3.25

    x.x = 5
    x.y = 5.5
    f(x)
    assert x.x == 5
    assert x.y == 5.5

g()
