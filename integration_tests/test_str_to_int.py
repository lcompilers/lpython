from ltypes import i32

def f():
    i: i32
    i = int("314")
    assert i == 314
    i = int("-314")
    assert i == -314

f()