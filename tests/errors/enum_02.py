from ltypes import i32, ccall
from enum import Enum

@ccall
class Color(Enum):
    RED: i32 = 1
    GREEN: i32 = 1
    BLUE: i32 = 2

@ccall
def test_selected_color(selected_color: Color) -> i32:
    pass

def test_colors():
    color: Color = Color(Color.BLUE)
    assert test_selected_color(color) == 2
    color = Color(Color.GREEN)
    assert test_selected_color(color) == 1
    color = Color(Color.RED)
    assert test_selected_color(color) == 1

test_colors()
