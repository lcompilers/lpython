from lpython import Const, i32, f64

from lpdraw import Line, Circle, Clear, Display
from lpconvexhull import convex_hull
from numpy import empty, int32

def plot_graph(polygon: list[tuple[i32, i32]], points: list[tuple[i32, i32]]):
    Width: Const[i32] = 500 # x-axis limits [0, 499]
    Height: Const[i32] = 500 # y-axis limits [0, 499]
    Screen: i32[Height, Width] = empty((Height, Width), dtype=int32)
    Clear(Height, Width, Screen)

    i: i32
    n: i32 = len(polygon)
    for i in range(n):
        Line(Height, Width, Screen, polygon[i][0], polygon[i][1], polygon[(i + 1) % n][0], polygon[(i + 1) % n][1])

    point_size: i32 = 5
    for i in range(len(points)):
        Circle(Height, Width, Screen, points[i][0], points[i][1], f64(point_size))

    Display(Height, Width, Screen)

def main0():
    points: list[tuple[i32, i32]] = [(445, 193), (138, 28), (418, 279), (62, 438), (168, 345), (435, 325), (293, 440), (158, 94), (403, 288), (136, 278), (141, 243), (287, 313), (338, 492), (172, 78), (29, 404), (79, 377), (184, 91), (69, 324), (408, 72), (494, 1)]
    convex_hull_points: list[tuple[i32, i32]] = convex_hull(points)
    # print(convex_hull_points)
    plot_graph(convex_hull_points, points)

    assert convex_hull_points == [(29, 404), (138, 28), (494, 1), (435, 325), (338, 492), (62, 438)]

main0()
