from lpython import i32, f64, TypeVar
import numpy

H = TypeVar("H")
W = TypeVar("W")

def Pixel(H: i32, W: i32, Screen: i32[H, W], x: i32, y: i32) -> None:
    if x >= 0 and y >= 0 and x < W and y < H:
        Screen[H - 1 - y, x] = 255

def Clear(H: i32, W: i32, Screen: i32[H, W]):
    i: i32
    j: i32
    for i in range(H):
        for j in range(W):
            Screen[i, j] = 0

def DisplayTerminal(H: i32, W: i32, Screen: i32[H, W]):
    i: i32
    j: i32

    print("+", end = "")
    for i in range(W):
        print("-", end = "")
    print("+")

    for i in range(H):
        print("|", end = "")
        for j in range(W):
            if bool(Screen[i, j]):
                print(".", end = "")
            else:
                print(" ", end = "")
        print("|")

    print("+", end = "")
    for i in range(W):
        print("-", end = "")
    print("+")

def Display(H: i32, W: i32, Screen: i32[H, W]):
    i: i32
    j: i32

    print("P2")
    print(W, H)
    print(255)

    for i in range(H):
        for j in range(W):
            print(Screen[i, j])

def Line(H: i32, W: i32, Screen: i32[H, W], x1: i32, y1: i32, x2: i32, y2: i32) -> None:
    dx: i32 = abs(x2 - x1)
    dy: i32 = abs(y2 - y1)

    sx: i32
    sy: i32

    if x1 < x2:
        sx = 1
    else:
        sx = -1

    if y1 < y2:
        sy = 1
    else:
        sy = -1

    err: i32 = dx - dy

    while x1 != x2 or y1 != y2:
        Pixel(H, W, Screen, x1, y1)
        e2: i32 = 2 * err

        if e2 > -dy:
            err -= dy
            x1 += sx

        if x1 == x2 and y1 == y2:
            Pixel(H, W, Screen, x1, y1)
            break

        if e2 < dx:
            err += dx
            y1 += sy

def Circle(H: i32, W: i32, Screen: i32[H, W], x: i32, y: i32, r: f64) -> None:
    x0: i32 = i32(r)
    y0: i32 = 0
    err: i32 = 0

    while x0 >= y0:
        Pixel(H, W, Screen, x + x0, y + y0)
        Pixel(H, W, Screen, x - x0, y + y0)
        Pixel(H, W, Screen, x + x0, y - y0)
        Pixel(H, W, Screen, x - x0, y - y0)
        Pixel(H, W, Screen, x + y0, y + x0)
        Pixel(H, W, Screen, x - y0, y + x0)
        Pixel(H, W, Screen, x + y0, y - x0)
        Pixel(H, W, Screen, x - y0, y - x0)

        if err <= 0:
            y0 += 1
            err += 2 * y0 + 1
        if err > 0:
            x0 -= 1
            err -= 2 * x0 + 1
