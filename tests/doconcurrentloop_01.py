from lpython import f32, i32

def triad(a: f32[:], b: f32[:], scalar: f32, c: f32[:]):
    N: i32
    i: i32
    N = size(a)
    for i in range(N): # type: parallel
        c[i] = a[i] + scalar * b[i]

def main0():
    a: f32[10000]
    b: f32[10000]
    c: f32[10000]
    scalar: f32
    i: i32
    nsize: i32
    scalar = f32(10.0)
    nsize = size(a)
    for i in range(nsize): # type: parallel
        a[i] = f32(5.0)
        b[i] = f32(5.0)
    triad(a, b, scalar, c)
    print("End Stream Triad")

main0()
