from ltypes import i32, f64

# Test: Printing ListConstant
def f():
    a: list[str] = ["ab", "abc", "abcd"]
    b: list[i32] = [1, 2, 3, 4]
    c: list[f64] = [1.23 , 324.3 , 56.431, 90.5, 34.1]
    d: list[i32] = []

    print(a)
    print(b)
    print(c)
    print(d)

    # print list constant
    print([-3, 2, 1, 0])
    print(['a', 'b', 'c', 'd' , 'e', 'f'])

f()
