from ltypes import f32, f64, c64

def func02(a: f32, b: f64, c: c64) -> c64:
    return complex(float(a), b) + c

def test_arg_passing():
    arg3: f32; arg4: f64; arg5: c64;

    arg3 = 3.0
    arg4 = float(4.0)
    arg5 = complex(5.0, 5.0)

    func02(arg3, arg4, c=arg5, b=arg4)

test_arg_passing()
