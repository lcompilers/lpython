from lpython import TypeVar, restriction, i32, f32
from numpy import empty

n = TypeVar("n")
m = TypeVar("m")
T = TypeVar('T')

@restriction
def add(x: T, y: T) -> T:
    pass

def add_integer(x: i32, y: i32) -> i32:
    return x + y

def add_float(x: f32, y: f32) -> f32:
    return x + y

def g(n: i32, m: i32, a: T[n,m], b: T[n,m], **kwargs) -> T[n,m]:
    r: T[n,m]
    r = empty([n,m])
    i: i32
    j: i32
    for i in range(n):
      for j in range(m):
        r[i,j] = add(a[i,j],b[i,j])
    print(r[0,0])

def main():
    a_int: i32[1,1] = empty([1,1])
    a_int[0,0] = 400
    b_int: i32[1,1] = empty([1,1])
    b_int[0,0] = 20
    g(1, 1, a_int, b_int, add=add_integer)
    a_float: f32[1,1] = empty([1,1])
    a_float[0,0] = f32(400)
    b_float: f32[1,1] = empty([1,1])
    b_float[0,0] = f32(20)
    g(1, 1, a_float, b_float, add=add_float)

main()
