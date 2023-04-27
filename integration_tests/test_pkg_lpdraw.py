from lpdraw import Line, Circle, Clear, Display, DisplayTerminal

from lpython import i32, TypeVar
from numpy import empty, int32

H = TypeVar("H")
W = TypeVar("W")

def test_screen(H: i32, W: i32, Screen: i32[H, W]):
    i: i32
    j: i32
    cnt: i32 = 0
    for i in range(H):
        for j in range(W):
            cnt += (Screen[i, j] - 256)

    assert cnt == -979630


def main():
    Width: i32 = 100 # x-axis limits [0, 99]
    Height: i32 = 40 # y-axis limits [0, 39]
    Screen: i32[40, 100] = empty((40, 100), dtype=int32)

    Clear(Height, Width, Screen)
    Line(Height, Width, Screen, 2, 4, 99, 11)
    Line(Height, Width, Screen, 0, 39, 49, 0)
    Circle(Height, Width, Screen, 52, 20, 6.0)
    Display(Height, Width, Screen)
    DisplayTerminal(Height, Width, Screen)

    test_screen(Height, Width, Screen)

main()
