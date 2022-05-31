from ltypes import i64

def fib(n: i64) -> i64:
    if n < 2:
        return n
    else:
        return fib(n - 1) + fib(n - 2)

def main0():
    ans: i64
    ans = fib(15)
    assert ans == 610

def main():
    # test of issue-529
    ans: i64
    ans = fib(10)
    assert ans == 55


main0()
main()
