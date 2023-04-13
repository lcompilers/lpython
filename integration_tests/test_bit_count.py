from ltypes import i8, i32, i16

def ff():
    assert 0 .bit_count() == 0
    assert 1 .bit_count() == 1
    assert 5 .bit_count() == 2
    assert -5 .bit_count() == -2

def ff1():
    x: i32
    x = 1 << 30
    assert x.bit_count() == 1

def ff2():
    x: i8
    x = i8(1 << 6)
    assert i32(x.bit_count()) == 1

def ff3():
    x: i16
    one: i16
    one = i16(1)
    x = -i16(one << i16(13))
    assert i32(x.bit_count()) == 1

ff()
ff1()
ff2()
ff3()