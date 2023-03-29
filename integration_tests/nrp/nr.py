from lpython import i32, f64


def func(x: f64, c: f64) -> f64:
    return x**2.0 - c**2.0


def func_prime(x: f64) -> f64:
    return 2.0*x


def newton_raphson(x: f64, c: f64, maxiter: i32) -> f64:
    h: f64
    err: f64 = 1e-5
    i: i32 = 0
    while abs(func(x, c)) > err and i < maxiter:
        h = func(x, c) /  func_prime(x)
        x -= h
        i += 1
    return x
