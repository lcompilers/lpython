from lpython import TypeVar, restriction, i32, f32
from numpy import empty, int32, float32

n = TypeVar("n")
T = TypeVar('T')

@restriction
def add(x: T, y: T) -> T:
    pass

def add_integer(x: i32, y: i32) -> i32:
    return x + y

def add_float(x: f32, y: f32) -> f32:
    return x + y

def g(n: i32, a: T[n], b: T[n], **kwargs):
  r: T[n]
  r = empty(n, dtype=object)
  i: i32
  for i in range(n):
    r[i] = add(a[i], b[i])
  print(r[0])

def main():
    a_int: i32[1] = empty(1, dtype=int32)
    a_int[0] = 400
    b_int: i32[1] = empty(1, dtype=int32)
    b_int[0] = 20
    g(1, a_int, b_int, add=add_integer)
    a_float: f32[1] = empty(1, dtype=float32)
    a_float[0] = f32(400.0)
    b_float: f32[1] = empty(1, dtype=float32)
    b_float[0] = f32(20.0)
    g(1, a_float, b_float, add=add_float)

main()
