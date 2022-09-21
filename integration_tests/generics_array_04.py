from ltypes import TypeVar, restriction, i32
from numpy import empty

n: i32
n = TypeVar("n")
m: i32
m = TypeVar("m")
T = TypeVar('T')

@restriction
def add(x: T, y: T) -> T:
    pass
    
def g(n: i32, m: i32, a: T[n,m], b: T[n,m]) -> T[n,m]:
    return add(a,b)
    
def main():
    a: i32[1,1] = empty([1,1])
    b: i32[1,1] = empty([1,1])
    g(1, 1, a, b)

main()