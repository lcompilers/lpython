import numpy as np
from typing import Callable


print(...)

# TODO: Make this work
# def bar(x = ...):
#     return x

array = np.random.rand(2, 2, 2, 2)
print(array[..., 0])
print(array[Ellipsis, 0])


def test1():
    ...


def test2() -> None:
    x = [1, [2, [...], 3]]
    l = [..., 1, 2, 3]
    ...


array = np.random.rand(2, 2, 2, 2)
print(array[..., 0])


def foo(x: ...) -> None:
    ...


def inject(get_next_item: Callable[..., str]) -> None:
    ...
