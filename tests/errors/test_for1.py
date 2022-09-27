def f():
    i: i32
    j: i32 = -1
    # increment should be compile time constant
    for i in range(0, -10, j):
        print(i)

f()
