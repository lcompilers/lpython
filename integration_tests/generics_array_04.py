T = TypeVar('T', bound=SupportsPlus)

def g(n: i32, m: i32, a: T[n,m], b: T[n,m]) -> T[n,m]:
    return a + b
    
def main():
    a: i32[1,1] = empty([1,1])
    b: i32[1,1] = empty([1,1])
    g(1, 1, a, b)

main()