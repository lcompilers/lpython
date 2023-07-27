from lpython import i32, Callable

def bar(func : Callable[[i32], i32], arg : i32) -> i32:
    return func(arg)
