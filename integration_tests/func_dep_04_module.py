from lpython import i32

def manhattan_distance(x1: i32, y1: i32, x2: i32, y2: i32) -> i32:
    return abs(x1 - x2) + abs(y1 - y2)
