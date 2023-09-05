from lpython import i16
from numpy import empty, int16

# Checks dim mismatch for global array variables
x: i16[4] = empty([5], dtype=int16)
