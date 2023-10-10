from lpython import Const, i32, f32, f64

foo: Const[i32] = 4
bar: Const[i32] = foo // 2

print(bar)
assert bar == 2

def floordiv1():
    a: f64
    b: f64
    c: f64
    a = 5.0
    b = 2.0
    c = a // b

    print(c)
    assert c == 2.0

def floordiv2():
    a: Const[f32] = f32(5.0)
    b: Const[f32] = f32(2.0)
    c: f32
    c = a // b

    print(c)
    assert c == f32(2.0)

def floordiv3():
    a: f64
    b: f64
    c: f64
    a = 5.0
    b = -2.0
    c = a // b

    print(c)
    assert c == -3.0

floordiv1()
floordiv2()
floordiv3()
