from ltypes import i32, f32

def f():
    i: i32
    i = 1 if True else 0
    assert i == 1
    j: f32
    j = 1.0 if 1.0 <= 0.0 else 0.0
    assert j == 0.0

f()
