from ltypes import i64

def fib(n: i64) -> i64:
    if n < 2:
        return n
    else:
        return fib(n - 1) + fib(n - 2)

def main():
    ans: i64
    x: i64
    x = 8
    ans = fib(x)
    print(ans)

main()
