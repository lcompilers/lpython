from lpython import i64

def fib(n: i64) -> i64:
    if n < i64(2):
        return n
    else:
        return fib(n - i64(1)) + fib(n - i64(2))

def main():
    ans: i64
    ans = fib(35, 10)
    print(ans)

main()
