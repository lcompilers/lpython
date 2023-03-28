from lpython import i32, f64, Const
from numpy import empty, int32

Width: Const[i32] = 100 # x-axis limits [0, 99]
Height: Const[i32] = 40 # y-axis limits [0, 39]
Screen: i32[Height, Width] = empty((Height, Width), dtype=int32)

def Pixel(x: i32, y: i32) -> None:
    Screen[y, x] = 255

def Clear():
    i: i32
    j: i32
    for i in range(Height):
        for j in range(Width):
            Screen[i, j] = 0

def DisplayTerminal():
    i: i32
    j: i32

    print("+", end = "")
    for i in range(Width):
        print("-", end = "")
    print("+")

    for i in range(Height):
        print("|", end = "")
        for j in range(Width):
            ch: str
            ch = "." if Screen[i, j] else " "
            print(ch, end = "")
        print("|")

    print("+", end = "")
    for i in range(Width):
        print("-", end = "")
    print("+")

def Display():
    i: i32
    j: i32

    print("P2")
    print(Width, Height)
    print(255)

    for i in range(Height):
        for j in range(Width):
            print(Screen[i, j])

def Line(x1: i32, y1: i32, x2: i32, y2: i32) -> None:
    dx: i32 = abs(x2 - x1)
    dy: i32 = abs(y2 - y1)
    sx: i32 = -1 if x1 > x2 else 1
    sy: i32 = -1 if y1 > y2 else 1
    err: i32 = dx - dy

    while x1 != x2 or y1 != y2:
        Pixel(x1, y1)
        e2: i32 = 2 * err
        if e2 > -dy:
            err -= dy
            x1 += sx
        if e2 < dx:
            err += dx
            y1 += sy
    Pixel(x2, y2)

def Circle(x: i32, y: i32, r: f64) -> None:
    x0: i32 = i32(int(r))
    y0: i32 = 0
    err: i32 = 0

    while x0 >= y0:
        Pixel(x + x0, y + y0)
        Pixel(x - x0, y + y0)
        Pixel(x + x0, y - y0)
        Pixel(x - x0, y - y0)
        Pixel(x + y0, y + x0)
        Pixel(x - y0, y + x0)
        Pixel(x + y0, y - x0)
        Pixel(x - y0, y - x0)

        if err <= 0:
            y0 += 1
            err += 2 * y0 + 1
        if err > 0:
            x0 -= 1
            err -= 2 * x0 + 1
