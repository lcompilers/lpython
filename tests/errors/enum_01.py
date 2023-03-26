from lpython import f64, ccall
from enum import Enum

@ccall
class Color(Enum):
    RED: f64 = 0.1
    GREEN: f64 = 0.2
    BLUE: f64 = 0.7

@ccall
def test_selected_color(selected_color: Color) -> f64:
    pass

def test_colors():
    color: Color = Color(Color.BLUE)
    assert test_selected_color(color) == 0.7
    color = Color(Color.GREEN)
    assert test_selected_color(color) == 0.2
    color = Color(Color.RED)
    assert test_selected_color(color) == 0.1

test_colors()
