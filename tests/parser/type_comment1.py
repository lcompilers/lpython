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
