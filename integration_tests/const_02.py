from ltypes import i32, f64, Const

def f(x: Const[i32]) -> i32:
    return x + 1

def g(x: Const[i32]) -> Const[f64]:
    return float(x + 2)

def h(x: i32) -> Const[i32]:
    return x + 3

def test_call_cases():
    y: i32 = 1
    yconst: Const[i32] = 4
    # argument const, parameter non-const
    print(h(yconst), f(y), g(yconst))
    assert h(yconst) == 7

    # argument non-const, parameter const
    assert f(y) == 2

    # argument const, parameter const
    assert g(yconst) == 6.0

def test_assign_cases():
    y: f64
    yconst: Const[i32] = 4
    # target const, value non-const - error case

    # target non-const, value const
    y = g(yconst)
    print(y)
    assert y == 6.0

    # target const, value const - error case

test_call_cases()
test_assign_cases()
