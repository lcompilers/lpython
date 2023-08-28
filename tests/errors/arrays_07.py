from lpython import f32
from numpy import empty, complex64

# checks type mismatch for types apart from integers
def main0():
    x: f32[5, 4] = empty([5, 4], dtype=complex64)

main0()
