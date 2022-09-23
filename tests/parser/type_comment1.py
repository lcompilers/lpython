import pytest  # type: ignore

def ndarray_func(x):
    # type: (np.ndarray) -> np.ndarray
    return x

@decorator1 # type: ignore

# Comment

@decorator2

# Comment

@decorator3 # type: ignore

def test(x):
    # type: (np.ndarray) -> np.ndarray
    return x

def test():  # type: ignore
    # Comment
    ...

def main():

    # type: ignore
    pass

x (x, # type: ignore
y)

from sympy.simplify import (collect, powsimp,  # type: ignore
    separatevars, simplify)

await test(  # type: ignore
x, y, z)
