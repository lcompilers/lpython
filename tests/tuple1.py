from ltypes import i32, f32

def test_Tuple():
    a1: tuple[i32, i32, i32]
    a1 = (1, 2, 3)
    a1 = (-3, -2, -1)

    a2: tuple[str, str, str]
    a2 = ("a", "b", "c")

    a3: tuple[i32, i32, f32, str]
    float_mem: f32
    float_mem = f32(0.45)
    a3 = (-2, -1, float_mem, "d")

    a4: tuple[tuple[i32, i32, i32], tuple[i32, i32, i32]]
    a4 = ((1, 2, 3), (4, 5, 6))

    a5: tuple[tuple[str, str, f32], tuple[str, i32, f32]]
    float_mem1: f32
    float_mem2: f32
    float_mem1 = f32(3.4)
    float_mem2 = f32(5.6)
    a5 = (("a", "b", float_mem1), ("c", 3, float_mem2))

    b0: i32
    b1: i32
    b0 = a1[0]
    b0, b1 = a1[2], a1[1]

    a11: tuple[i32, i32]
    b11: tuple[i32, i32]
    a11 = (1, 2)
    b11 = (1, 2)
    assert a11 == b11
