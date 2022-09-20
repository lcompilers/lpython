from ltypes import i32
from enum import Enum

class Color(Enum):
    RED: i32 = 1
    GREEN: i32 = 2
    BLUE: i32 = 3

def test_color_enum():
    print(Color.RED.value, Color.GREEN.value, Color.BLUE.value)
    assert Color.RED.value == 1
    assert Color.GREEN.value == 2
    assert Color.BLUE.value == 3

def test_selected_color(selected_color: Color):
    color: Color
    color = Color(selected_color)
    assert color == Color.RED
    print(color.name)

test_color_enum()
test_selected_color(Color.RED)
