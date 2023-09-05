from lpython import i16
from numpy import empty, int32

# Checks type mismatch for global array variables
x: i16[5] = empty([5], dtype=int32)
