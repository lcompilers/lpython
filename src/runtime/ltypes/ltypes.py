from inspect import getfullargspec, getcallargs
from typing import types

# data-types

i32 = types.new_class("i32")
i64 = types.new_class("i64")
f32 = types.new_class("f32")
f64 = types.new_class("f64")
c32 = types.new_class("c32")
c64 = types.new_class("c64")

# overloading support

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
    raise Exception("Unsupported Type")


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
            raise Exception("Function not defined")
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
        raise Exception("Function not found with matching signature")


def overload(f):
    overloaded_f = OverloadedFunction(f)
    return overloaded_f
