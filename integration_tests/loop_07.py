from lpython import i32

def main0():
    points: list[tuple[i32, i32]] = [(445, 193), (138, 28), (418, 279)]
    point: tuple[i32, i32]
    x_sum: i32 = 0
    y_sum: i32 = 0
    for point in points:
        print(point)
        x_sum += point[0]
        y_sum += point[1]

    print(x_sum, y_sum)
    assert x_sum == 1001
    assert y_sum == 500

main0()
