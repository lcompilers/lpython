from lpython import f64

def sqr(x: f64) -> f64:
    return x * x

def computeCircleArea(radius: f64) -> f64:
    pi: f64 = 3.14
    return pi * sqr(radius)

def main0():
    x: f64 = 213.512
    y: f64 = 55.23
    print(x)
    print(y)
    print(x + y)
    print(x * y)
    print(x - y)
    print(x / y)
    print(computeCircleArea(5.0))
    print(computeMean(2.0, 7.0, 10.0))

def computeMean(a: f64, b: f64, c: f64) -> f64:
    return (a + b + c) / 3.0

main0()
