from ltypes import f64

def f(a: f64[:], b: f64[:]) -> f64[size(a)]:
    r: f64[size(a)]
    i: i32
    for i in range(size(a)):
        r[i] = a[i] + b[i]
    return r

def main():
    a: f64[1] = empty(1)
    b: f64[1] = empty(1)
    f(a, b)