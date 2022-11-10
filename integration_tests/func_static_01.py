from ltypes import i64, static

@static
def fib(n: i64) -> i64:
    if n < i64(2):
        return n
    return fib(n - i64(1)) + fib(n - i64(2))

def main0():
    ans: i64; x: i64
    x = i64(5)
    ans = fib(x)
    assert ans == i64(5)

main0()
