from ltypes import inline, i32

@inline
def add(x: i32, y: i32) -> i32:
    return x + y

def main0():
    x: i32; y: i32
    x = (2+3)*5
    y = add(x, 2)*2
    assert x == 25
    assert y == 54

main0()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()
