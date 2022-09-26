from ltypes import i64, static

@static
def fib(n: i64) -> i64:
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

def main0():
    ans: i64; x: i64
    x = 5
    ans = fib(x)
    assert ans == 5

main0()
