from lpython import f64
import random

def test_seed():
    random.seed()
    t1: f64 = random.random()
    t2: f64 = random.random()
    print(t1, t2)
    assert abs(t1 - t2) > 1e-3

test_seed()
