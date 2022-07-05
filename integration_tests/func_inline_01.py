from ltypes import i64

def fib(n: i64) -> i64:
    if n < 2:
        return n
    return fib(n - 1) + fib(n - 2)

def main():
    ans: i64
    x: i64
    x = 40
    ans = fib(x)
    print(ans)
    assert ans == 102334155

main()
