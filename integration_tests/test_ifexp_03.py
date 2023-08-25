from lpython import i32

def fib(n: i32) -> i32:
    return fib(n - 1) + fib(n - 2) if n >= 3 else 1

def main0():
    res: i32 = fib(30)
    print(res)
    assert res == 832040

main0()
