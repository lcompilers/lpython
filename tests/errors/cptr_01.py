from lpython import CPtr, empty_c_void_p

def f():
    x: CPtr = empty_c_void_p()
    print(x[0])

f()
