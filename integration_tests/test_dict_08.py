# test case for passing dict as args and return value to a function

from lpython import i32

def get_cubes_from_squares(squares: dict[i32, i32]) -> dict[i32, i32]:
    i : i32
    cubes: dict[i32, i32] = {}
    for i in range(1, 16):
        cubes[i] = squares[i] * i
    return cubes

def assert_dict(squares: dict[i32, i32], cubes: dict[i32, i32]):
    i : i32
    for i in range(1, 16):
        assert squares[i] == (i * i)
    for i in range(1, 16):
        assert cubes[i] == (i * i * i)
    assert len(squares) == 15
    assert len(cubes) == 15

def test_dict():
    squares : dict[i32, i32]
    squares = {1:1, 2:4, 3:9, 4:16, 5:25, 6:36, 7:49,
               8:64, 9:81, 10:100, 11:121, 12:144, 13:169,
               14: 196, 15:225}
    
    cubes : dict[i32, i32]
    cubes = get_cubes_from_squares(squares)
    assert_dict(squares, cubes)

test_dict()
