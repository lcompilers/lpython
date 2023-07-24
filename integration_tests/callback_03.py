from lpython import i32, Callable

def foo(x : i32) -> None:
    print(x)
    assert x == 3

def bar(func : Callable[[i32], None], arg : i32) -> i32:
    func(arg)

def main0():
    bar(foo, 3)

main0()
