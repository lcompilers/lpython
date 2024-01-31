from lpython import ccall, CPtr, p_c_pointer, pointer, i64, empty_c_void_p, Out
import os

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_new_stack(x: CPtr) -> None:
    pass

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_new_heap() -> CPtr:
    pass

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_const_pi(x: CPtr) -> None:
    pass

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_assign(x: CPtr, y:CPtr) -> None:
    pass

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_str(x: CPtr) -> str:
    pass

@ccall(header="symengine/cwrapper.h", c_shared_lib="symengine", c_shared_lib_path=f"{os.environ['CONDA_PREFIX']}/lib")
def basic_free_stack(x: CPtr) -> None:
    pass

def mmrv(r: Out[list[CPtr]]) -> None:
    # x: S = pi
    _x: i64 = i64(0)
    x: CPtr = empty_c_void_p()
    p_c_pointer(pointer(_x, i64), x)
    basic_new_stack(x)
    basic_const_pi(x)

    # l1: list[S]
    l1: list[CPtr] = []

    # l1 = [x]
    i: i32 = 0
    Len: i32 = 1
    for i in range(Len):
        tmp: CPtr = basic_new_heap()
        l1.append(tmp)
        basic_assign(l1[0], x)

    # print(l1[0])
    s1: str = basic_str(l1[0])
    print(s1)
    assert s1 == "pi"

    # r = l1
    r = l1

    basic_free_stack(x)

def test_mrv():
    # ans : list[S]
    # temp : list[S]
    ans: list[CPtr] = []
    temp: list[CPtr] = []

    # mmrv(ans)
    # temp = ans
    mmrv(ans)
    temp = ans

    # print(temp[0])
    s2: str = basic_str(temp[0])
    print(s2)
    assert s2 == "pi"

test_mrv()