from ltypes import i32, i64, f32, f64, c64, overload

def func01(a: i32, b: i64) -> i64:
    return int(a) + b

def test_arg_passing():
    arg1: i32;

    arg1 = 1

    func01(arg1, d=int(8))

test_arg_passing()
