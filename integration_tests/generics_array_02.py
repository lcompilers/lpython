from ltypes import TypeVar, restriction, i32
from numpy import empty

n: i32
n = TypeVar("n")
T = TypeVar('T')

@restriction
def add(x: T, y: T) -> T:
    pass

def g(n: i32, a: T[n], b: T[n]) -> T[n]:
    return add(a,b)

def main():
    a: i32[1] = empty(1)
    b: i32[1] = empty(1)
    g(1, a, b)

main()