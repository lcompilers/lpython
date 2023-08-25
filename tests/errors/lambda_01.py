
from lpython import i32, Callable

def main0():
    x: Callable[[i32, i32, i32], i32] = lambda p, q, r, s: p +  q + r + s

    a123 = x(1, 2, 3)

main0()
