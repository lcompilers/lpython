from lpython import i32, f64

def min(points: list[tuple[i32, i32]]) -> tuple[i32, i32]:
    """Finds the left-most point in a list of points.

    Args:
        points: A list of points.

    Returns:
        The left-most point in the list.
    """

    left_most_point: tuple[i32, i32] = points[0]
    point: tuple[i32, i32]
    for point in points:
        if point[0] < left_most_point[0]:
            left_most_point = point

    return left_most_point


def distance(p: tuple[i32, i32], q: tuple[i32, i32]) -> f64:
    # Function to calculate the Euclidean distance between two points
    x1: i32; y1: i32
    x2: i32; y2: i32

    x1, y1 = p
    x2, y2 = q
    return f64((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
