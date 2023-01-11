from ltypes import i32

def fib(n: i32) -> i32:
    if n <= 1:
        return n
    return fib(n - 1) + fib(n - 2)

def printNums(n: i32):
    if (n == 0):
        return
    print(n)
    printNums(n - 1)

def main0():
    print("===========================")
    i: i32
    for i in range(15):
        print(fib(i))
    print("===========================")
    print(fib(20))
    print("===========================")
    printNums(10)

main0()
