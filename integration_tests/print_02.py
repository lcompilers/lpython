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

def test_nested_lists():
    w: list[list[list[list[list[f64]]]]] = [[[[[2.13, -98.17]]], [[[1.11]]]]]
    x: list[list[list[i32]]] = [[[3, -1], [-2, 5], [5]], [[-3, 1], [2, -5], [-5]]]
    y: list[list[f64]] = [[3.14, -1.0012], [-2.38, 5.51]]
    z: list[list[str]] = [["bat", "ball"], ["cat", "dog"], ["c", "c++", "java", "python"]]

    print(w)
    print(x)
    print(y)
    print(z)

f()
test_nested_lists()
