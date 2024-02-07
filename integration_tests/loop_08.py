from lpython import i32, Const

def main0():
    i: i32
    n: Const[i32] = 10
    M2: Const[i32] = 2
    y: i32 = 0
    for i in range(0, n, M2):  # each M2 block in A cols and B rows  # !!!!!!!!!!!!!!
        y = y + 2
    print(y)
    assert(y == 10)

main0()
