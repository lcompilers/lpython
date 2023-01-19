from ltypes import i32, f64

def compI32(x: i32, y: i32):
    print(x)
    print(y)
    if x < y:
        print("x lt y")
    if x > y:
        print("x gt y")
    if x == y:
        print("x eq y")
    if x >= y:
        print("x ge y")
    if x <= y:
        print("x le y")
    if x != y:
        print("x ne y")

def compF64(x: f64, y: f64):
    print(x)
    print(y)
    if x < y:
        print("x lt y")
    if x > y:
        print("x gt y")
    if x == y:
        print("x eq y")
    if x >= y:
        print("x ge y")
    if x <= y:
        print("x le y")
    if x != y:
        print("x ne y")

def main0():
    a: i32 = 4
    b: i32 = 5
    c: i32 = 5
    d: i32 = 6
    compI32(a, b)
    compI32(b, a)
    compI32(a, d)
    compI32(b, c)
    compI32(c, b)

    x: f64 = 4.555
    y: f64 = 15.55
    w: f64 = 15.55
    z: f64 = 25.12
    compF64(x, y)
    compF64(y, x)
    compF64(x, z)
    compF64(y, w)
    compF64(w, y)

main0()
