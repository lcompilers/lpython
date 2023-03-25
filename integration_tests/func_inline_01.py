from lpython import i64

def fib(n: i64) -> i64:
    if n < i64(2):
        return n
    return fib(n - i64(1)) + fib(n - i64(2))

def main():
    ans: i64
    x: i64
    x = i64(40)
    ans = fib(x)
    print(ans)
    assert ans == i64(102334155)

main()
