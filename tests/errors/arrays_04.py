from lpython import i16
from numpy import empty, int32

# checks type mismatch for local array variable
def main0():
    x: i16[5] = empty([5], dtype=int32)

main0()
