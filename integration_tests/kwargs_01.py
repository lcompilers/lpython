from ltypes import i32, i64, f32, f64, c64, overload

def func01(a: i32, b: i64) -> i64:
    return int(a) + b

@overload
def func02(a: f32, b: f64, c: c64) -> c64:
    return complex(float(a), b) + c

@overload
def func02(a: i64, b: i32, c: f64, s: str) -> str:
    return str(a) + "_" + str(b) + "_" + str(int(c)) + s

def test_arg_passing():
    arg1: i32; arg2: i64;
    arg3: f32; arg4: f64;
    arg5: c64; arg6: str;
    result: c64; eps: f64 = 1e-12;

    arg1 = 1
    arg2 = int(2)
    arg3 = 3.0
    arg4 = float(4.0)
    arg5 = complex(5.0, 5.0)
    arg6 = "testing"

    assert func01(arg1, arg2) == int(3)
    assert func01(6, int(arg1)) == int(7)
    assert func01(a=arg1, b=arg2) == int(3)
    assert func01(b=arg2, a=arg1) == int(3)
    assert func01(7, b=arg2) == int(9)

    result = func02(arg3, arg4, arg5)
    assert abs(result.real - 8.0) <= eps
    assert abs(result.imag - 9.0) <= eps
    result = func02(a=arg3, b=arg4, c=arg5)
    assert abs(result.real - 8.0) <= eps
    assert abs(result.imag - 9.0) <= eps
    result = func02(c=arg5, b=arg4, a=arg3)
    assert abs(result.real - 8.0) <= eps
    assert abs(result.imag - 9.0) <= eps
    result = func02(arg3, c=arg5, b=arg4)
    assert abs(result.real - 8.0) <= eps
    assert abs(result.imag - 9.0) <= eps
    result = func02(arg3, arg4, c=arg5)
    assert abs(result.real - 8.0) <= eps
    assert abs(result.imag - 9.0) <= eps

    print(func02(int(10), arg1, s="testing123", c=11.0))
    assert func02(int(10), arg1, s="testing123", c=11.0) == "10_1_11testing123"
    assert func02(int(10), arg1, c=arg4, s=arg6) == "10_1_4testing"

test_arg_passing()
