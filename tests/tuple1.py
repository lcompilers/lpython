def test_Tuple():
    a1: tuple[i32, i32, i32]
    a1 = (1, 2, 3)
    a1 = (-3, -2, -1)

    a2: tuple[str, str, str]
    a2 = ("a", "b", "c")

    a3: tuple[i32, i32, f32, str]
    a3 = (-2, -1, 0.45, "d")

    a4: tuple[tuple[i32, i32, i32], tuple[i32, i32, i32]]
    a4 = ((1, 2, 3), (4, 5, 6))

    a5: tuple[tuple[str, str, f32], tuple[str, i32, f32]]
    a5 = (("a", "b", 3.4), ("c", 3, 5.6))

    b0: i32
    b1: i32
    b0 = a1[0]
    b0, b1 = a1[2], a1[1]
