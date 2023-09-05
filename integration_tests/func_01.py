from lpython import i32, InOut

def reserve(a: InOut[list[i32]], b: i32):
    a.append(b)
    print("user defined reserve() called")

def main0():
    x: list[i32] = []
    reserve(x, 5)

    assert len(x) == 1
    assert x[0] == 5

main0()
