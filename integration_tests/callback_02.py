from lpython import ccallback, i32, Callable

# test issue 2169

def foo(x : i32) -> i32:
    return x**2

def bar(func : Callable[[i32], i32], arg : i32) -> i32:
    return func(arg)

@ccallback
def entry_point() -> None:
    z: i32 = 5
    x: i32 = bar(foo, z)
    assert z**2 == x


entry_point()
