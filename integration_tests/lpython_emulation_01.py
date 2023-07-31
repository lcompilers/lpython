import lpython_emulation_01_mod
import lpython
from lpython import ccall, i64
from types import FunctionType
lpython.CTypes.emulations = {k: v for k, v in \
        lpython_emulation_01_mod.__dict__.items() \
        if isinstance(v, FunctionType)}

@ccall
def f1(a: i64) -> i64:
    pass

@ccall
def f2(a: i64):
    pass

def main():
    assert f1(2) == 3
    f2(4)

main()
