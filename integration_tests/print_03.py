from lpython import i32

def Main0():
    x: i32
    x = (2+3)* (-5)
    y: i32 = 100
    z: i32 = 2147483647
    # w: i32 = -2147483648 # ultimately we should support printing this
    w: i32 = -2147483647
    print(x)
    print(y)
    print(z)
    print(w)
    print("Hi")
    print("Hello")
    print((5-2) * 7)
    print("Bye")

Main0()
