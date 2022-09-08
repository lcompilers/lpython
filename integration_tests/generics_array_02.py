T = TypeVar('T', bound=SupportsPlus)

def g(n: i32, a: T[n], b: T[n]) -> T[n]:
    return a + b

def main():
    a: i32[1] = empty(1)
    b: i32[1] = empty(1)
    g(1, a, b)

main()