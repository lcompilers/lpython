from inspect import getfullargspec, getcallargs
import os
import ctypes
import platform
from typing import TypeVar

__slots__ = ["i8", "i16", "i32", "i64", "f32", "f64", "c32", "c64",
        "overload", "ccall", "TypeVar"]

# data-types

class Type:
    def __init__(self, name):
        self._name = name

    def __getitem__(self, params):
        return Array(self, params)

class Array:
    def __init__(self, type, dims):
        self._type = type
        self._dims = dims

i8 = Type("i8")
i16 = Type("i16")
i32 = Type("i32")
i64 = Type("i64")
f32 = Type("f32")
f64 = Type("f64")
c32 = Type("c32")
c64 = Type("c64")

# Overloading support

def ltype(x):
    """
    Converts CPython types to LPython types
    """
    if type(x) == int:
        return i32, i64
    elif type(x) == float:
        return f32, f64
    elif type(x) == complex:
        return c32, c64
    elif type(x) == str:
        return (str, )
    elif type(x) == bool:
        return (bool, )
    raise Exception("Unsupported Type: %s" % str(type(x)))


class OverloadedFunction:
    """
    A wrapper class for allowing overloading.
    """
    global_map = {}

    def __init__(self, func):
        self.func_name = func.__name__
        f_list = self.global_map.get(func.__name__, [])
        f_list.append((func, getfullargspec(func)))
        self.global_map[func.__name__] = f_list

    def __call__(self, *args, **kwargs):
        func_map_list = self.global_map.get(self.func_name, False)
        if not func_map_list:
            raise Exception("Function: %s is not defined" % self.func_name)
        for item in func_map_list:
            func, key = item
            try:
                # This might fail for the cases when arguments don't match
                ann_dict = getcallargs(func, *args, **kwargs)
            except TypeError:
                continue
            flag = True
            for k, v in ann_dict.items():
                if not key.annotations.get(k, False):
                    flag = False
                    break
                else:
                    if not (key.annotations.get(k) in ltype(v)):
                        flag = False
                        break
            if flag:
                return func(*args, **kwargs)
        raise Exception(f"Function: {self.func_name} not found with matching "
                        "signature")


def overload(f):
    overloaded_f = OverloadedFunction(f)
    return overloaded_f


def interface(f):
    def inner_func():
        raise Exception("Unexpected to be called by CPython")
    return inner_func


# C interoperation support

class CTypes:
    """
    A wrapper class for interfacing C via ctypes.
    """

    def __init__(self, f):
        def convert_type_to_ctype(arg):
            if arg == f64:
                return ctypes.c_double
            elif arg == f32:
                return ctypes.c_float
            elif arg == i64:
                return ctypes.c_int64
            elif arg == i32:
                return ctypes.c_int32
            elif arg is None:
                raise NotImplementedError("Type cannot be None")
            elif isinstance(arg, Array):
                type = convert_type_to_ctype(arg._type)
                return ctypes.POINTER(type)
            else:
                raise NotImplementedError("Type not implemented")
        def get_rtlib_dir():
            current_dir = os.path.dirname(os.path.abspath(__file__))
            return os.path.join(current_dir, "..")
        def get_crtlib_name():
            if platform.system() == "Linux":
                return "liblpython_runtime.so"
            elif platform.system() == "Darwin":
                return "liblpython_runtime.dylib"
            elif platform.system() == "Windows":
                return "lpython_runtime.dll"
            else:
                raise NotImplementedError("Platform not implemented")
        def get_crtlib_path():
            return os.path.join(get_rtlib_dir(), get_crtlib_name())
        self.name = f.__name__
        self.args = f.__code__.co_varnames
        self.annotations = f.__annotations__
        crtlib = get_crtlib_path()
        self.library = ctypes.CDLL(crtlib)
        self.cf = self.library[self.name]
        argtypes = []
        for arg in self.args:
            arg_type = self.annotations[arg]
            arg_ctype = convert_type_to_ctype(arg_type)
            argtypes.append(arg_ctype)
        self.cf.argtypes = argtypes
        if "return" in self.annotations:
            res_type = self.annotations["return"]
            if res_type is not None:
                self.cf.restype = convert_type_to_ctype(res_type)

    def __call__(self, *args, **kwargs):
        if len(kwargs) > 0:
            raise Exception("kwargs are not supported")
        return self.cf(*args)


def ccall(f):
    wrapped_f = CTypes(f)
    return wrapped_f
