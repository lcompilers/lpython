from ltypes import i32, ccall
from enum import Enum

@ccall
class Color(Enum):
    RED: i32 = 0
    GREEN: i32 = 1
    BLUE: i32 = 2
    PINK: i32 = 3
    WHITE: i32 = 4
    YELLOW: i32 = 5

@ccall
def test_selected_color(selected_color: Color) -> i32:
    pass

def test_colors():
    color: Color = Color(Color.BLUE)
    assert test_selected_color(color) == 2
    color = Color(Color.GREEN)
    assert test_selected_color(color) == 1
    color = Color(Color.RED)
    assert test_selected_color(color) == 0
    color = Color(Color.YELLOW)
    assert test_selected_color(color) == 5
    color = Color(Color.WHITE)
    assert test_selected_color(color) == 4
    color = Color(Color.PINK)
    assert test_selected_color(color) == 3

test_colors()
