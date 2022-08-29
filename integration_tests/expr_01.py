from ltypes import inline, i32

@inline
def add(x: i32, y: i32) -> i32:
    return x + y

@inline
def and_op(x: i32, y: i32) -> i32:
    return x & y

def main0():
    x: i32
    y: i32
    z: i32
    x = (2+3)*5
    y = add(x, 2)*2
    assert x == 25
    assert y == 54

    z = and_op(x, y)
    assert z == 16


main0()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()
