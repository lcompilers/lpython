import numpy as np
from typing import Callable

array = np.random.rand(2, 2, 2, 2)
print(array[..., 0])
print(array[Ellipsis, 0])

def inject(get_next_item: Callable[..., str]) -> None:
    ...

def foo(x: ...) -> None:
    ...

class flow:
    def __understand__(self, name: str, value: ...) -> None: ...

def foo(x = ...):
    return x

def test():
    ...

class Todo:
    ...

x = [1, [2, [...], 3]]
l = [..., 1, 2, 3]

if x is ...:
    pass

def partial(func: Callable[..., str], *args) -> Callable[..., str]:
    pass

class xyz:
    abc: str = ...
    def __init__(self, name: str=...) -> None: ...
