from lpython import ccall, Pointer, i32, i64, empty_c_void_p, CPtr, pointer

@ccall(header="Python.h")
def Py_Initialize():
    pass

@ccall(header="Python.h")
def Py_IsInitialized() -> i32:
    pass

@ccall(header="Python.h")
def Py_DecodeLocale(s: str, p: CPtr) -> CPtr:
    pass

@ccall(header="Python.h")
def PySys_SetArgv(n: i32, args: Pointer[CPtr]):
    pass

@ccall(header="Python.h")
def Py_FinalizeEx() -> i32:
    pass

@ccall(header="Python.h")
def PyUnicode_FromString(s: str) -> CPtr:
    pass

@ccall(header="Python.h")
def PyImport_Import(name: CPtr) -> CPtr:
    pass

@ccall(header="Python.h")
def Py_DecRef(name: CPtr):
    pass

@ccall(header="Python.h")
def Py_IncRef(name: CPtr):
    pass

@ccall(header="Python.h")
def PyObject_GetAttrString(m: CPtr, s: str) -> CPtr:
    pass

@ccall(header="Python.h")
def PyTuple_New(n: i32) -> CPtr:
    pass

@ccall(header="Python.h")
def PyObject_CallObject(a: CPtr, b: CPtr) -> CPtr:
    pass

@ccall(header="Python.h")
def PyLong_AsLongLong(a: CPtr) -> i32:
    pass
