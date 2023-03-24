from lpython import i64

def fib(n: i64) -> i64:
    if n < i64(2):
        return n
    else:
        return fib(n - i64(1)) + fib(n - i64(2))

def main0():
    ans: i64
    ans = fib(i64(15))
    assert ans == i64(610)

def main():
    # test of issue-529
    ans: i64
    ans = fib(i64(10))
    assert ans == i64(55)


main0()
main()
