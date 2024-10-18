import numpy
from numpy import empty

from lpython import (i16)


def main():
    A_nm: i16[15, 3] = empty((15, 3), dtype=numpy.int16)
    print ("hello, world!")


if __name__ == "__main__":
    main()
