# Out of bound
from ltypes import i32

def test_list():
    x: list[i32]
    x = [1, 2, 3]
    x[3] = 0

test_list()
