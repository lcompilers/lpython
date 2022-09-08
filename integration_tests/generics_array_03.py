T = TypeVar('T', bound=SupportsPlus)

def g(a: T[:], b: T[:]) -> T:
    r: T[size(a)]
    i: i32
    for i in range(size(a)):
        r[i] = a[i] + b[i]
    return r

def main():
    a: i32[1] = empty(1)
    b: i32[1] = empty(1)
    g(a, b)

main()