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
    x = A()
    x.x = 5
    x.y = 5.5
    f(x)

g()
