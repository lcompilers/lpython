from ltypes import ccall, f32, f64, i32, i64

@ccall
def f(x: f64) -> f64:
    pass

@ccall
def g(a: f64, b: f32, c: i64, d: i32) -> None:
    pass
