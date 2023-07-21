from lpython import ccall, CPtr, p_c_pointer, pointer, i64, empty_c_void_p
import os

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_new_stack(x: CPtr) -> None:
    pass

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_const_pi(x: CPtr) -> None:
    pass

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_str(x: CPtr) -> str:
    pass

def main0():
    y: i64 = i64(0)
    x: CPtr = empty_c_void_p()
    p_c_pointer(pointer(y), x)
    basic_new_stack(x)
    basic_const_pi(x)
    s: str = basic_str(x)
    print(s)
    assert s == "pi"

main0()