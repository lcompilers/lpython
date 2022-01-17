def test_bin():
    b: str
    b = bin(5)
    b = bin(64)

def test_complex():
    c: c128
    c1: c128
    c2: c128
    c3: c128
    b: bool

    # constant real or int as args
    c = complex()
    c = complex(3.4)
    c = complex(5., 4.3)
    c = complex(1)
    c1 = complex(3, 4)
    c2 = complex(2, 4.5)
    c3 = complex(3., 4.)
    b = c1 != c2
    b = c1 == c3

def test_namedexpr():
    a: i32
    x: i32
    y: i32

    # check 1
    x = (y := 0)

    # check 2
    if a := ord('3'):
        x = 1

    # check 3
    while a := 1:
        y = 1

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
