from func_dep_04_module import manhattan_distance
from lpython import i32

def test_manhattan_distance():
    a: i32; b: i32; c: i32; d: i32;
    a = 2
    b = 3
    c = 4
    d = 5
    assert manhattan_distance(a, b, c, d) == 4
    assert manhattan_distance(a, c, b, d) == 2

test_manhattan_distance()
