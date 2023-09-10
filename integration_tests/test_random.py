from lpython import i32, f64
import random


def test_random():
    r: f64
    r = random.random()
    assert r >= 0.0 and r < 1.0
    r = random.random()
    assert r >= 0.0 and r < 1.0

def test_randrange():
    r: i32
    r = random.randrange(0, 10) # [0, 10)
    assert r >= 0 and r < 10
    r = random.randrange(-50, 76) # [-50, 76)
    assert r >= -50 and r < 76

def test_randint():
    ri1: i32
    ri2: i32
    ri1 = random.randint(0, 10) # [0, 10]
    assert ri1 >= 0 and ri1 <= 10
    ri2 = random.randint(-50, 76) # [-50, 76]
    assert ri2 >= -50 and ri2 <= 76

def test_uniform():
    r: f64
    r = random.uniform(5., 76.)
    assert r >= 5. and r <= 76.
    r = random.uniform(-50., 76.)
    assert r >= -50. and r <= 76.

def test_paretovariate():
    r: f64
    r = random.paretovariate(2.0)
    print(r)
    r = random.paretovariate(-5.6)
    print(r)

def test_expovariate():
    r: f64
    r = random.expovariate(2.0)
    print(r)
    r = random.expovariate(-5.6)
    print(r)

def test_weibullvariate():
    r: f64
    r = random.weibullvariate(2.0, 3.0)
    print(r)
    r = random.weibullvariate(-5.6, 1.2)
    print(r)

def test_seed():
    random.seed(123)
    t1: f64
    t1 = random.random()
    random.seed(321)
    t2: f64
    t2 = random.random()
    assert t1 != t2

def check():
    test_random()
    test_randrange()
    test_randint()
    test_uniform()
    test_paretovariate()
    test_expovariate()
    test_weibullvariate()
    test_seed()

check()
