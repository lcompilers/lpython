from math import floor, log2
from ltypes import i8, i32, i16

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
    x = i8(1 << 6)
    assert i32(x.bit_length()) == 7

def ff3():
    x: i16
    one: i16
    one = i16(1)
    x = -i16(one << i16(13))
    assert i32(x.bit_length()) == 14


ff()
ff1()
ff2()
ff3()
