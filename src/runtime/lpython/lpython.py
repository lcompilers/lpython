from inspect import getfullargspec, getcallargs, isclass, getsource
import os
import ctypes
import platform
from dataclasses import dataclass as py_dataclass, is_dataclass as py_is_dataclass

# TODO: this does not seem to restrict other imports
__slots__ = ["i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64", "c32", "c64", "CPtr",
        "overload", "ccall", "TypeVar", "pointer", "c_p_pointer", "Pointer",
        "p_c_pointer", "vectorize", "inline", "Union", "static",
        "packed", "Const", "sizeof", "ccallable", "ccallback", "Callable",
        "Allocatable"]

# data-types

type_to_convert_func = {
    "i8": int,
    "i16": int,
    "i32": int,
    "i64": int,
    "u8": lambda x: x,
    "u16": lambda x: x,
    "u32": lambda x: x,
    "u64": lambda x: x,
    "f32": float,
    "f64": float,
    "c32": complex,
    "c64": complex,
    "c_ptr": lambda x: x,
    "Const": lambda x: x,
    "Callable": lambda x: x,
    "Allocatable": lambda x: x,
    "Pointer": lambda x: x,
}

class Type:
    def __init__(self, name):
        self._name = name
        self._convert = type_to_convert_func[name]

    def __getitem__(self, params):
        return Array(self, params)

    def __call__(self, arg):
        return self._convert(arg)

def dataclass(arg):
    def __class_getitem__(key):
        return Array(arg, key)
    arg.__class_getitem__ = __class_getitem__

    return py_dataclass(arg)

def is_ctypes_Structure(obj):
    return (isclass(obj) and issubclass(obj, ctypes.Structure))

def is_dataclass(obj):
    return ((isclass(obj) and issubclass(obj, ctypes.Structure)) or
             py_is_dataclass(obj))

class PointerType(Type):
    def __getitem__(self, type):
        if is_dataclass(type):
            return convert_to_ctypes_Structure(type)
        return type

class ConstType(Type):
    def __getitem__(self, type):
        return type

class Array:
    def __init__(self, type, dims):
        self._type = type
        self._dims = dims

i8 = Type("i8")
i16 = Type("i16")
i32 = Type("i32")
i64 = Type("i64")
u8 = Type("u8")
u16 = Type("u16")
u32 = Type("u32")
u64 = Type("u64")
f32 = Type("f32")
f64 = Type("f64")
c32 = Type("c32")
c64 = Type("c64")
CPtr = Type("c_ptr")
Const = ConstType("Const")
Callable = Type("Callable")
Allocatable = Type("Allocatable")
Union = ctypes.Union
Pointer = PointerType("Pointer")


class Intent:
    def __init__(self, type):
        self._type = type

    def __getitem__(self, params):
        return params

In = Intent("In")
Out = Intent("Out")
InOut = Intent("InOut")

# Generics

class TypeVar():
    def __init__(self, name):
        self._name = name

    def __getitem__(self, params):
        return Array(self, params)

def restriction(func):
    return func

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
    overloaded_f.__name__ = f.__name__
    overloaded_f.__code__ = f.__code__
    overloaded_f.__annotations__ = f.__annotations__
    return overloaded_f

# To be handled in ASR
def vectorize(f):
    return f

# To be handled in backend
def inline(f):
    return f

# To be handled in backend
def static(f):
    return f

class PackedDataClass:
    pass

def packed(*args, aligned=None):
    if len(args) == 1:
        if not is_dataclass(args[0]):
            raise TypeError("packed can only be applied over a dataclass.")
        class PackedDataClassLocal(args[0], PackedDataClass):
            class_to_pack = args[0]
        return PackedDataClassLocal

    def _packed(f):
        if not is_dataclass(f):
            raise TypeError("packed can only be applied over a dataclass.")
        class PackedDataClassLocal(f, PackedDataClass):
            class_to_pack = f
        return PackedDataClassLocal
    return _packed

def interface(f):
    def inner_func():
        raise Exception("Unexpected to be called by CPython")
    return inner_func


# C interoperation support

class c_complex(ctypes.Structure):
    def __eq__(self, other):
        if isinstance(other, complex):
            return self.real == other.real and self.imag == other.imag
        elif isinstance(other, (int, float)):
            return self.real == other and self.imag == 0.0
        return super().__eq__(other)

    def __sub__(self, other):
        import numpy as np
        if isinstance(other, (complex, np.complex64, np.complex128)):
            return complex(self.real - other.real, self.imag - other.imag)
        elif isinstance(other, (int, float)):
            return complex(self.real - other, self.imag)
        raise NotImplementedError()

class c_float_complex(c_complex):
    _fields_ = [("real", ctypes.c_float), ("imag", ctypes.c_float)]

class c_double_complex(c_complex):
    _fields_ = [("real", ctypes.c_double), ("imag", ctypes.c_double)]

def convert_type_to_ctype(arg):
    from enum import Enum
    if arg == f64:
        return ctypes.c_double
    elif arg == f32:
        return ctypes.c_float
    elif arg == i64:
        return ctypes.c_int64
    elif arg == i32:
        return ctypes.c_int32
    elif arg == i16:
        return ctypes.c_int16
    elif arg == i8:
        return ctypes.c_int8
    elif arg == u64:
        return ctypes.c_uint64
    elif arg == u32:
        return ctypes.c_uint32
    elif arg == u16:
        return ctypes.c_uint16
    elif arg == u8:
        return ctypes.c_uint8
    elif arg == CPtr:
        return ctypes.c_void_p
    elif arg == str:
        return ctypes.c_char_p
    elif arg == c32:
        return c_float_complex
    elif arg == c64:
        return c_double_complex
    elif arg == bool:
        return ctypes.c_bool
    elif arg is None:
        raise NotImplementedError("Type cannot be None")
    elif isinstance(arg, Array):
        if is_dataclass(arg._type):
            return arg
        type = convert_type_to_ctype(arg._type)
        return ctypes.POINTER(type)
    elif is_dataclass(arg):
        return convert_to_ctypes_Structure(arg)
    elif issubclass(arg, Enum):
        # TODO: store enum in ctypes.Structure with name and value as fields.
        return ctypes.c_int64
    else:
        raise NotImplementedError("Type %r not implemented" % arg)

def convert_numpy_dtype_to_ctype(arg):
    import numpy as np
    if arg == np.float64:
        return ctypes.c_double
    elif arg == np.float32:
        return ctypes.c_float
    elif arg == np.int64:
        return ctypes.c_int64
    elif arg == np.int32:
        return ctypes.c_int32
    elif arg == np.int16:
        return ctypes.c_int16
    elif arg == np.int8:
        return ctypes.c_int8
    elif arg == np.uint64:
        return ctypes.c_uint64
    elif arg == np.uint32:
        return ctypes.c_uint32
    elif arg == np.uint16:
        return ctypes.c_uint16
    elif arg == np.uint8:
        return ctypes.c_uint8
    elif arg == np.void:
        return ctypes.c_void_p
    elif arg is None:
        raise NotImplementedError("Type cannot be None")
    else:
        raise NotImplementedError("Type %r not implemented" % arg)

class CTypes:
    """
    A wrapper class for interfacing C via ctypes.
    """

    def __init__(self, f):
        def get_rtlib_dir():
            current_dir = os.path.dirname(os.path.abspath(__file__))
            return os.path.join(current_dir, "..")
        def get_lib_name(name):
            if platform.system() == "Linux":
                return "lib" + name + ".so"
            elif platform.system() == "Darwin":
                return "lib" + name + ".dylib"
            elif platform.system() == "Windows":
                return name + ".dll"
            else:
                raise NotImplementedError("Platform not implemented")
        def get_crtlib_path():
            py_mod = os.environ.get("LPYTHON_PY_MOD_NAME", "")
            if py_mod == "":
                return os.path.join(get_rtlib_dir(),
                    get_lib_name("lpython_runtime"))
            else:
                py_mod_path = os.environ["LPYTHON_PY_MOD_PATH"]
                return os.path.join(py_mod_path, get_lib_name(py_mod))
        self.name = f.__name__
        self.args = f.__code__.co_varnames
        self.annotations = f.__annotations__
        if "LPYTHON_PY_MOD_NAME" in os.environ:
            crtlib = get_crtlib_path()
            self.library = ctypes.CDLL(crtlib)
            self.cf = self.library[self.name]
        else:
            self.cf = CTypes.emulations[self.name]
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
        new_args = []
        for arg in args:
            import numpy as np
            if isinstance(arg, str):
                new_args.append(arg.encode("utf-8"))
            elif isinstance(arg, np.ndarray):
                new_args.append(arg.ctypes.data_as(ctypes.POINTER(convert_numpy_dtype_to_ctype(arg.dtype))))
            else:
                new_args.append(arg)
        return self.cf(*new_args)

def convert_to_ctypes_Union(f):
    fields = []
    for name in f.__annotations__:
        ltype_ = f.__annotations__[name]
        fields.append((name, convert_type_to_ctype(ltype_)))

    f._fields_ = fields
    f.__annotations__ = {}

    return f

def get_fixed_size_of_array(ltype_: Array):
    if isinstance(ltype_._dims, tuple):
        size = 1
        for dim in ltype_._dims:
            if not isinstance(dim, int):
                return None
            size *= dim
    elif isinstance(ltype_._dims, int):
        return ltype_._dims
    return None

def convert_to_ctypes_Structure(f):
    fields = []

    pack_class = issubclass(f, PackedDataClass)
    if pack_class:
        f = f.class_to_pack

    if not issubclass(f, ctypes.Structure):
        for name in f.__annotations__:
            ltype_ = f.__annotations__[name]
            if isinstance(ltype_, Array):
                array_size = get_fixed_size_of_array(ltype_)
                if array_size is not None:
                    ltype_ = ltype_._type
                    fields.append((name, convert_type_to_ctype(ltype_) * array_size))
                else:
                    fields.append((name, convert_type_to_ctype(ltype_)))
            else:
                fields.append((name, convert_type_to_ctype(ltype_)))
    else:
        fields = f._fields_
        pack_class = pack_class or f._pack_


    class ctypes_Structure(ctypes.Structure):
        _pack_ = int(pack_class)
        _fields_ = fields

        def __init__(self, *args):
            if len(args) != 0 and len(args) != len(self._fields_):
                super().__init__(*args)

            for field, arg in zip(self._fields_, args):
                from enum import Enum
                member = self.__getattribute__(field[0])
                value = arg
                if isinstance(member, ctypes.Array):
                    import numpy as np
                    if isinstance(value, np.ndarray):
                        if value.dtype == np.complex64:
                            value = value.flatten().tolist()
                            value = [c_float_complex(val.real, val.imag) for val in value]
                        elif value.dtype == np.complex128:
                            value = value.flatten().tolist()
                            value = [c_double_complex(val.real, val.imag) for val in value]
                        value = type(member)(*value)
                elif isinstance(value, Enum):
                    value = value.value
                self.__setattr__(field[0], value)

    ctypes_Structure.__name__ = f.__name__

    return ctypes_Structure

def ccall(f):
    if isclass(f) and issubclass(f, Union):
        return f
    return CTypes(f)

def union(f):
    fields = []
    for name in f.__annotations__:
        ltype_ = f.__annotations__[name]
        fields.append((name, convert_type_to_ctype(ltype_)))

    f._fields_ = fields
    f.__annotations__ = {}
    return f

def pointer(x, type_=None):
    if type_ is None:
        type_ = type(x)
    from numpy import ndarray
    if isinstance(x, ndarray):
        return x.ctypes.data_as(ctypes.POINTER(convert_numpy_dtype_to_ctype(x.dtype)))
    else:
        if type_ == i8:
            return ctypes.cast(ctypes.pointer(ctypes.c_int8(x)),
                    ctypes.c_void_p)
        elif type_ == i16:
            return ctypes.cast(ctypes.pointer(ctypes.c_int16(x)),
                    ctypes.c_void_p)
        elif type_ == i32:
            return ctypes.cast(ctypes.pointer(ctypes.c_int32(x)),
                    ctypes.c_void_p)
        elif type_ == i64:
            return ctypes.cast(ctypes.pointer(ctypes.c_int64(x)),
                    ctypes.c_void_p)
        elif type_ == u8:
            return ctypes.cast(ctypes.pointer(ctypes.c_uint8(x)),
                    ctypes.c_void_p)
        elif type_ == u16:
            return ctypes.cast(ctypes.pointer(ctypes.c_uint16(x)),
                    ctypes.c_void_p)
        elif type_ == u32:
            return ctypes.cast(ctypes.pointer(ctypes.c_uint32(x)),
                    ctypes.c_void_p)
        elif type_ == u64:
            return ctypes.cast(ctypes.pointer(ctypes.c_uint64(x)),
                    ctypes.c_void_p)
        elif type_ == f32:
            return ctypes.cast(ctypes.pointer(ctypes.c_float(x)),
                    ctypes.c_void_p)
        elif type_ == f64:
            return ctypes.cast(ctypes.pointer(ctypes.c_double(x)),
                    ctypes.c_void_p)
        elif is_dataclass(type_):
            if issubclass(type_, ctypes.Structure):
                return ctypes.cast(ctypes.pointer(x), ctypes.c_void_p)
            else:
                return x
        else:
            raise Exception("Type not supported in pointer()")

class PointerToStruct:

    def __init__(self, ctypes_ptr_):
        self.__dict__["ctypes_ptr"] = ctypes_ptr_

    def __getattr__(self, name: str):
        if name == "ctypes_ptr":
            return self.__dict__[name]
        value = self.ctypes_ptr.contents.__getattribute__(name)
        if isinstance(value, (c_float_complex, c_double_complex)):
            value = complex(value.real, value.imag)
        return value

    def __setattr__(self, name: str, value):
        name_ = self.ctypes_ptr.contents.__getattribute__(name)
        from enum import Enum
        if isinstance(name_, c_float_complex):
            if isinstance(value, complex):
                value = c_float_complex(value.real, value.imag)
            else:
                value = c_float_complex(value.real, 0.0)
        elif isinstance(name_, c_double_complex):
            if isinstance(value, complex):
                value = c_double_complex(value.real, value.imag)
            else:
                value = c_double_complex(value.real, 0.0)
        elif isinstance(name_, ctypes.Array):
            import numpy as np
            if isinstance(value, np.ndarray):
                if value.dtype == np.complex64:
                    value = value.flatten().tolist()
                    value = [c_float_complex(val.real, val.imag) for val in value]
                elif value.dtype == np.complex128:
                    value = value.flatten().tolist()
                    value = [c_double_complex(val.real, val.imag) for val in value]
                value = type(name_)(*value)
        elif isinstance(value, Enum):
            value = value.value
        self.ctypes_ptr.contents.__setattr__(name, value)

def c_p_pointer(cptr, targettype):
    targettype_ptr = convert_type_to_ctype(targettype)
    if isinstance(targettype, Array):
        if py_is_dataclass(targettype._type):
            return ctypes.cast(cptr.value, ctypes.py_object).value
        newa = ctypes.cast(cptr, targettype_ptr)
        return newa
    else:
        if py_is_dataclass(targettype):
            if cptr.value is None:
                return None
            return ctypes.cast(cptr, ctypes.py_object).value

        targettype_ptr = ctypes.POINTER(targettype_ptr)
        newa = ctypes.cast(cptr, targettype_ptr)
        if is_ctypes_Structure(targettype):
            # return after wrapping newa inside PointerToStruct
            return PointerToStruct(newa)
        return newa

def p_c_pointer(ptr, cptr):
    if isinstance(ptr, ctypes.c_void_p):
        cptr.value = ptr.value
    else:
        # assign the address of ptr in memory to cptr.value
        # the case for numpy arrays converted to a pointer
        cptr.value = id(ptr)

def empty_c_void_p():
    class ctypes_c_void_p(ctypes.c_void_p):

        def __eq__(self, value):
            return self.value == value.value

        def __repr__(self):
            return str(self.value)

    return ctypes_c_void_p()

def sizeof(arg):
    return ctypes.sizeof(convert_type_to_ctype(arg))

def ccallable(f):
    if py_is_dataclass(f):
        return convert_to_ctypes_Structure(f)
    return f

def ccallback(f):
    return f

class lpython:
    """
    The @lpython decorator compiles a given function using LPython.

    The decorator should be used from CPython mode, i.e., when the module is
    being run using CPython. When possible, it is recommended to use LPython
    for the main program, and use the @cpython decorator from the LPython mode
    to access CPython features that are not supported by LPython.
    """

    def __init__(self, function):
        def get_rtlib_dir():
            current_dir = os.path.dirname(os.path.abspath(__file__))
            return os.path.join(current_dir, "..")

        def get_type_info(arg):
            # return_type -> (`type_format`, `variable type`, `array struct name`)
            # See: https://docs.python.org/3/c-api/arg.html for more info on type_format
            if arg == f64:
                return ('d', "double", 'r64')
            elif arg == f32:
                return ('f', "float", 'r32')
            elif arg == i64:
                return ('l', "long int", 'i64')
            elif arg == i32:
                return ('i', "int", 'i32')
            elif arg == bool:
                return ('p', "bool", '')
            elif isinstance(arg, Array):
                t = get_type_info(arg._type)
                if t[2] == '':
                    raise NotImplementedError("Type %r not implemented" % arg)
                return ('O', ["PyArrayObject *", "struct "+t[2]+" *", t[1]+" *"], '')
            else:
                raise NotImplementedError("Type %r not implemented" % arg)

        def get_data_type(t):
            if isinstance(t, list):
                return t[0]
            else:
                return t + " "

        self.fn_name = function.__name__
        # Get the source code of the function
        source_code = getsource(function)
        source_code = source_code[source_code.find('\n'):]

        dir_name = "./lpython_decorator_" + self.fn_name
        if not os.path.exists(dir_name):
            os.mkdir(dir_name)
        filename = dir_name + "/" + self.fn_name

        # Open the file for writing
        with open(filename + ".py", "w") as file:
            # Write the Python source code to the file
            file.write("@ccallable")
            file.write(source_code)
        # ----------------------------------------------------------------------
        types = function.__annotations__
        self.arg_type_formats = ""
        self.return_type = ""
        self.return_type_format = ""
        self.arg_types = {}
        counter = 1
        for t in types.keys():
            if t == "return":
                type = get_type_info(types[t])
                self.return_type_format = type[0]
                self.return_type = type[1]
            else:
                type = get_type_info(types[t])
                self.arg_type_formats += type[0]
                self.arg_types[counter] = type[1]
                counter += 1
        # ----------------------------------------------------------------------
        # `arg_0`: used as the return variables
        # arguments are declared as `arg_1`, `arg_2`, ...
        variables_decl = ""
        if self.return_type != "":
            variables_decl = "// Declare return variables and arguments\n"
            variables_decl += "    " + get_data_type(self.return_type) + "arg_" \
                + str(0) + ";\n"
        # ----------------------------------------------------------------------
        # `PyArray_AsCArray` is used to convert NumPy Arrays to C Arrays
        # `fill_array_details` contains arrays operations to be
        # performed on the arguments
        # `parse_args` are used to capture the args from CPython
        # `pass_args` are the args that are passed to the shared library function
        fill_array_details = ""
        parse_args = ""
        pass_args = ""
        numpy_init = ""
        for i, t in self.arg_types.items():
            if i > 1:
                parse_args += ", "
                pass_args += ", "
            if isinstance(t, list):
                if numpy_init == "":
                    numpy_init = "// Initialize NumPy\n    import_array();\n\n    "
                fill_array_details += f"""\n
    // fill array details for args[{i-1}]
    if (PyArray_NDIM(arg_{i}) != 1) {{
        PyErr_SetString(PyExc_TypeError,
            "Only 1 dimension is implemented for now.");
        return NULL;
    }}

    {t[1]}s_array_{i} = malloc(sizeof(struct r64));
    {{
        {t[2]}array;
        // Create C arrays from numpy objects:
        PyArray_Descr *descr = PyArray_DescrFromType(PyArray_TYPE(arg_{i}));
        npy_intp dims[1];
        if (PyArray_AsCArray((PyObject **)&arg_{i}, (void *)&array, dims, 1, descr) < 0) {{
            PyErr_SetString(PyExc_TypeError, "error converting to c array");
            return NULL;
        }}

        s_array_{i}->data = array;
        s_array_{i}->n_dims = 1;
        s_array_{i}->dims[0].lower_bound = 0;
        s_array_{i}->dims[0].length = dims[0];
        s_array_{i}->is_allocated = false;
    }}"""
                pass_args += "s_array_" + str(i)
            else:
                pass_args += "arg_" + str(i)
            variables_decl += "    " + get_data_type(t) + "arg_" + str(i) + ";\n"
            parse_args += "&arg_" + str(i)

        if parse_args != "":
            parse_args = f"""\n    // Parse the arguments from Python
    if (!PyArg_ParseTuple(args, "{self.arg_type_formats}", {parse_args})) {{
        return NULL;
    }}"""

        # ----------------------------------------------------------------------
        # Handle the return variable if any; otherwise, return None
        fill_return_details = ""
        if self.return_type != "":
            fill_return_details = f"""\n\n    // Call the C function
    arg_0 = {self.fn_name}({pass_args});

    // Build and return the result as a Python object
    return Py_BuildValue("{self.return_type_format}", arg_0);"""
        else:
            fill_return_details = f"""{self.fn_name}({pass_args});
    Py_RETURN_NONE;"""

        # ----------------------------------------------------------------------
        # Python wrapper for the Shared library
        template = f"""// Python headers
#include <Python.h>

// NumPy C/API headers
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION // remove warnings
#include <numpy/ndarrayobject.h>

// LPython generated C code
#include "{self.fn_name}.h"

// Define the Python module and method mappings
static PyObject* define_module(PyObject* self, PyObject* args) {{
    {numpy_init}{variables_decl}{parse_args}\
{fill_array_details}{fill_return_details}
}}

// Define the module's method table
static PyMethodDef module_methods[] = {{
    {{"{self.fn_name}", define_module, METH_VARARGS,
        "Handle arguments & return variable and call the function"}},
    {{NULL, NULL, 0, NULL}}
}};

// Define the module initialization function
static struct PyModuleDef module_def = {{
    PyModuleDef_HEAD_INIT,
    "lpython_module_{self.fn_name}",
    "Shared library to use LPython generated functions",
    -1,
    module_methods
}};

PyMODINIT_FUNC PyInit_lpython_module_{self.fn_name}(void) {{
    PyObject* module;

    // Create the module object
    module = PyModule_Create(&module_def);
    if (!module) {{
        return NULL;
    }}

    return module;
}}
"""
        # ----------------------------------------------------------------------
        # Write the C source code to the file
        with open(filename + ".c", "w") as file:
            file.write(template)

        # ----------------------------------------------------------------------
        # Generate the Shared library
        # TODO: Use LLVM instead of C backend
        r = os.system("lpython --show-c --disable-main "
            + filename + ".py > " + filename + ".h")
        assert r == 0, "Failed to create C file"

        gcc_flags = ""
        if platform.system() == "Linux":
            gcc_flags = " -shared -fPIC "
        elif platform.system() == "Darwin":
            gcc_flags = " -bundle -flat_namespace -undefined suppress "
        else:
            raise NotImplementedError("Platform not implemented")

        from numpy import get_include
        from distutils.sysconfig import get_python_inc, get_python_lib
        python_path = "-I" + get_python_inc() + " "
        numpy_path = "-I" + get_include() + " "
        rt_path_01 = "-I" + get_rtlib_dir() + "/../libasr/runtime "
        rt_path_02 = "-L" + get_rtlib_dir() + " -Wl,-rpath " \
            + get_rtlib_dir() + " -llpython_runtime "
        python_lib = "-L" + get_python_lib() + "/../.. -lpython3.10 -lm"

        r = os.system("gcc -g" +  gcc_flags + python_path + numpy_path +
            filename + ".c -o lpython_module_" + self.fn_name + ".so " +
            rt_path_01 + rt_path_02 + python_lib)
        assert r == 0, "Failed to create the shared library"

    def __call__(self, *args, **kwargs):
        import sys; sys.path.append('.')
        # import the symbol from the shared library
        function = getattr(__import__("lpython_module_" + self.fn_name),
            self.fn_name)
        return function(*args, **kwargs)
