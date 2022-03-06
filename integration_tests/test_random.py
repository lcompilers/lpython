from ltypes import f64
import random


def test_random():
    r: f64
    r = random.random()
    print(r)
    assert r >= 0.0 and r < 1.0


test_random()