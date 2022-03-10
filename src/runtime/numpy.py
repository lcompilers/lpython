from ltypes import overload
from numpy_builtin import size

@overload
def sum(a: i32[:]) -> i32:
    s: i32
    i: i32
    s = 0
    for i in range(size(a)):
        s += a[i]
    return s

@overload
def sum(a: i32[:,:]) -> i32:
    s: i32
    i: i32
    j: i32
    s = 0
    for i in range(size(a,0)):
        for j in range(size(a,1)):
            s += a[i,j]
    return s

@overload
def sum(a: i32[:,:,:]) -> i32:
    s: i32
    i: i32
    j: i32
    k: i32
    s = 0
    for i in range(size(a,0)):
        for j in range(size(a,1)):
            for k in range(size(a,2)):
                s += a[i,j,k]
    return s
