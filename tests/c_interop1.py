from ltypes import ccall, f32, f64, i32, i64

@ccall
def f(x: f64) -> f64:
    pass

@ccall
def g(a: f64, b: f32, c: i64, d: i32) -> None:
    pass

@ccallback
def h(x: f64) -> f64:
    return x + 1.0

@ccallback
def l(a: f64, b: f32, c: i64, d: i32) -> None:
    print("OK")

def main0():
    i: f64
    x: f64
    x = 5.0
    i = f(x)
    y: f32
    y = f32(5.4)
    z: i64
    z = i64(3)
    zz: i32
    zz = 2
    g(x, y, z, zz)
    i = h(x)
    l(x, y, z, zz)
