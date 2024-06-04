from math import floor, log2
from lpython import i8, i32, i16

def ff():
    # assert -8 .bit_length() == -4
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

# def ff4():
#     print((-100).bit_length())
#     print((-4).bit_length())

#     assert (-100).bit_length() == 7
#     assert (-4).bit_length() == 3

# def ff5():
#     a: i32
#     a = 100
#     print((-a).bit_length())
#     print((-(-(-(-a)))).bit_length())

#     assert (-a).bit_length() == 7
#     assert (-(-(-(-a)))).bit_length() == 7

ff()
ff1()
ff2()
ff3()
ff4()
ff5()
