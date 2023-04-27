from lpython import i32

def key_not_present():
    x: dict[i32, i32] = {}
    x = {1: 1, 2: 2}
    print(x[10])

key_not_present()
