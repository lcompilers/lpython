from lpython import ccall

@ccall(header="symengine/cwrapper.h")
def basic_new_heap() -> CPtr:
    pass

@ccall(header="symengine/cwrapper.h")
def basic_const_pi(x: CPtr) -> None:
    pass

@ccall(header="symengine/cwrapper.h")
def basic_str(x: CPtr) -> str:
    pass

def main0():
    x: CPtr = basic_new_heap()
    basic_const_pi(x)
    print(basic_str(x))

main0()