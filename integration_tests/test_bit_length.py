from math import floor, log2
def ff():
    assert -8 .bit_length() == -4
    #TODO:(1 << 12).bit_length()
    #TODO:(anything return integeri).bit_length()
    assert 121212 .bit_length() == 17

def ff1():
    x: i32
    x = 1 << 30
    assert x.bit_length() == 31

def ff2():
    x: i8
    x = 1 << 6
    assert x.bit_length() == 7

def ff3():
    x: i16
    one: i16
    one = 1
    x = -(one << 13)
    assert x.bit_length() == 14


ff()
ff1()
ff2()
ff3()
