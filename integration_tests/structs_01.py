from ltypes import i32, f32, dataclass

@dataclass
class A:
    x: i32
    y: f32

def f(a: A):
    print(a.x)
    print(a.y)

def g():
    x: A
    x = A(3, 3.3)
    f(x)
    # TODO: the above constructor does not initialize `A` in LPython yet, so
    # the below does not work:
    #assert x.x == 3
    #assert x.y == 3.3

    x.x = 5
    x.y = 5.5
    f(x)
    assert x.x == 5
    assert x.y == 5.5

g()
