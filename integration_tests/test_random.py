from ltypes import i32, f64
import random


def test_random():
    r: f64
    r = random.random()
    print(r)
    assert r >= 0.0 and r < 1.0
    r = random.random()
    print(r)
    assert r >= 0.0 and r < 1.0

def test_randint():
    ri1: i32
    ri2: i32
    ri1 = random.randint(0, 10) # [0, 10]
    print(ri1)
    assert ri1 >= 0 and ri1 <= 10
    ri2 = random.randint(-50, 76) # [-50, 76]
    print(ri2)
    assert ri2 >= -50 and ri2 <= 76

test_random()
test_randint()
