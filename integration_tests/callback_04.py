import lpython
from lpython import i32
from types import FunctionType
import callback_04_module

lpython.CTypes.emulations = {k: v for k, v in callback_04_module.__dict__.items()
        if isinstance(v, FunctionType)}


def foo(x : i32) -> i32:
    assert x == 3
    print(x)
    return x

def entry_point() -> None:
    callback_04_module.bar(foo, 3)

entry_point()
