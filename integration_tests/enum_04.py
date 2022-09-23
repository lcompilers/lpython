from ltypes import i32
from enum import Enum, auto

class Color(Enum):
    RED: i32 = auto()
    GREEN: i32 = auto()
    BLUE: i32 = 7
    YELLOW: i32 = auto()
    WHITE: i32 = auto()
    PINK: i32 = 15
    GREY: i32 = auto()

class Integers(Enum):
    a: i32 = -5
    b: i32 = auto()
    c: i32 = auto()
    d: i32 = 0
    e: i32 = auto()
    f: i32 = 7
    g: i32 = auto()

def test_color_enum():
    print(Color.RED.value, Color.RED.name)
    print(Color.GREEN.value, Color.GREEN.name)
    print(Color.BLUE.value, Color.BLUE.name)
    print(Color.YELLOW.value, Color.YELLOW.name)
    print(Color.WHITE.value, Color.WHITE.name)
    print(Color.PINK.value, Color.PINK.name)
    print(Color.GREY.value, Color.GREY.name)
    assert Color.RED.value == 1
    assert Color.GREEN.value == 2
    assert Color.BLUE.value == 7
    assert Color.YELLOW.value == 8
    assert Color.WHITE.value == 9
    assert Color.PINK.value == 15
    assert Color.GREY.value == 16

def test_selected_color(selected_color: Color):
    color: Color
    color = Color(selected_color)
    assert color == Color.YELLOW
    print(color.name, color.value)

def test_integer(integer: Integers, value: i32):
    assert integer.value == value

def test_integers():
    print(Integers.a.name, Integers.a.value)
    test_integer(Integers.a, -5)

    print(Integers.b.name, Integers.b.value)
    test_integer(Integers.b, -4)

    print(Integers.c.name, Integers.c.value)
    test_integer(Integers.c, -3)

    print(Integers.d.name, Integers.d.value)
    test_integer(Integers.d, 0)

    print(Integers.e.name, Integers.e.value)
    test_integer(Integers.e, 1)

    print(Integers.f.name, Integers.f.value)
    test_integer(Integers.f, 7)

    print(Integers.g.name, Integers.g.value)
    test_integer(Integers.g, 8)

test_color_enum()
test_selected_color(Color.YELLOW)
test_integers()
