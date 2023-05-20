from lpython import i32
from .utils import min, distance

def orientation(p: tuple[i32, i32], q: tuple[i32, i32], r: tuple[i32, i32]) -> i32:
    # Function to find the orientation of triplet (p, q, r)
    # Returns the following values:
    # 0: Colinear
    # 1: Clockwise
    # 2: Counterclockwise
    value: i32 = (q[1] - p[1]) * (r[0] - q[0]) - (q[0] - p[0]) * (r[1] - q[1])
    if value == 0:
        return 0  # Colinear
    elif value > 0:
        return 1  # Clockwise
    else:
        return 2  # Counterclockwise


def convex_hull(points: list[tuple[i32, i32]]) -> list[tuple[i32, i32]]:
    """Finds the convex hull of a set of points.

    Args:
        points: A list of points.

    Returns:
        A list of points that form the convex hull.
    """

    n: i32 = len(points)
    if n < 3:
        return [(-1, -1)]  # Convex hull not possible

    # Find the leftmost point
    leftmost: tuple[i32, i32] = min(points)
    hull: list[tuple[i32, i32]] = []

    p: tuple[i32, i32] = leftmost

    while True:
        hull.append(p)
        q: tuple[i32, i32] = points[0]

        r: tuple[i32, i32]
        for r in points:
            if r == p or r == q:
                continue
            direction: i32 = orientation(p, q, r)
            if direction == 1 or (direction == 0 and distance(p, r) > distance(p, q)):
                q = r

        p = q

        if p == leftmost:
            break

    return hull
