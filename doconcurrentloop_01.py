def triad(a: f32[:], b: f32[:], scalar: f32, c: f32[:]):
    N: i32
    i: i32
    N = size(a)
    for i in range(N): # type: parallel
        c[i] = a[i] + scalar * b[i]

def main():
    a: f32[10000]
    b: f32[10000]
    c: f32[10000]
    scalar: f32
    i: i32
    nsize: i32
    scalar = 10
    nsize = size(a)
    for i in range(nsize): # type: parallel
        a[i] = 5
        b[i] = 5
    triad(a, b, scalar, c)
    print("End Stream Triad")
