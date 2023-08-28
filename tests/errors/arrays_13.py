from lpython import i16
from numpy import empty, int16

# checks dim mismatch for local array variable
# when dim is specified as a constant integer
def main0():
    x: i16[4] = empty(5, dtype=int16)

main0()
