from lpython import (f64,)

result : f64 = f64(14)
divisor : f64 = f64(4)
result /= divisor

assert abs(result - f64(3.5)) < 1e-12
