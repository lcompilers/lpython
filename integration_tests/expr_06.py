from ltypes import i32, f32
from numpy import empty

def main0():
    x: i32 = 25
    y: i32 = (2 + 3) * 5
    z: f32 = (2.0 + 3) * 5.0
    xa: i32[3] = empty(3)
    assert x == 25
    assert y == 25
    assert z == 25.0

main0()

# Not implemented yet in LPython:
#if __name__ == "__main__":
#    main()
