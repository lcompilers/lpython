# This file contains LPython implementation of some numpy functions
# It is tested by both CPython and LPython.
from ltypes import i32, i64, f64, TypeVar
from numpy import empty, int64

n: i32
n = TypeVar("n")

def zeros(n: i32) -> f64[n]:
    A: f64[n]
    A = empty(n)
    i: i32
    for i in range(n):
        A[i] = 0.0
    return A

def ones(n: i32) -> f64[n]:
    A: f64[n]
    A = empty(n)
    i: i32
    for i in range(n):
        A[i] = 1.0
    return A

def arange(n: i32) -> i64[n]:
    A: i64[n]
    A = empty(n, dtype=int64)
    i: i32
    for i in range(n):
        A[i] = i
    return A

num: i32
num = TypeVar("num")
def linspace(start: f64, stop: f64, num: i32) -> f64[num]:
    A: f64[num]
    A = empty(num)
    i: i32
    for i in range(num):
        A[i] = start + (stop-start)*i/(num-1)
    return A
