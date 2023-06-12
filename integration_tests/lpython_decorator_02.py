from numpy import array
from lpython import i32, f64, lpython

@lpython
def multiply(n: i32, x: f64[:]) -> f64[:]:
    i: i32
    for i in range(n):
        x[i] *= 5.0
    return x

def test():
    x: f64[3] = array([11.0, 12.0, 13.0, 14., 15.])
    print(multiply(5, x))

test()
