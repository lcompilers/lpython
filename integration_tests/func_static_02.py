from ltypes import i64, static, inline

@static
@inline
def fib(n: i64) -> i64:
    i: i64; t1: i64; t2: i64; nextTerm: i64;
    t1 = int(1)
    t2 = int(1)
    nextTerm = t1 + t2
    for i in range(int(3), n):
        t1 = t2
        t2 = nextTerm
        nextTerm = t1 + t2
    return nextTerm

def main0():
    ans: i64; x: i64
    x = 5
    ans = fib(x)
    print(ans)
    assert ans == 5

main0()
