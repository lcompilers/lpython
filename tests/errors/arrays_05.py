from lpython import i16
from numpy import empty, int16

# checks multi-dim mismatch for local array variable
def main0():
    x: i16[5, 4] = empty([5, 3], dtype=int16)

main0()
