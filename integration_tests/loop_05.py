def f():
    j :i32 = 0
    while True:
        j = j + 1
        print(j)
        if(j>4):
            break
    print("out of outer loop")

f()