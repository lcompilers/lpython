import sys
import os

intrinsic_funcs_args = {
    "Kind": [
        {
            "args": [("int",), ("real",), ("bool",), ("char",), (("complex",))],
            "return": "int32"
        },
    ],
    "FMA": [
        {
            "args": [("real", "real", "real")],
            "ret_type_arg_idx": 0
        }
    ],
    "FlipSign": [
        {
            "args": [("int", "real")],
            "ret_type_arg_idx": 1
        }
    ],
    "FloorDiv": [
        {
            "args": [("int", "int"), ("uint", "uint"), ("real", "real"), ("bool", "bool")],
            "ret_type_arg_idx": 0
        },
    ],
    "Mod": [
        {
            "args": [("int", "int"), ("real", "real")],
            "ret_type_arg_idx": 0
        },
    ],
    "Trailz": [
        {
            "args": [("int",)],
            "ret_type_arg_idx": 0
        },
    ],
    "Modulo": [
        {
            "args": [("int", "int"), ("real", "real")],
            "ret_type_arg_idx": 0
        },
    ],
    "BesselJ0": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0
        },
    ],
    "BesselJ1": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0
        },
    ],
    "BesselY0": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0
        },
    ],
    "Mvbits": [
        {
            "args": [("int", "int", "int", "int", "int")],
            "ret_type_arg_idx": 3
        },
    ],
    "Leadz": [
        {
            "args": [("int",)],
            "ret_type_arg_idx": 0
        },
    ],
    "ToLowerCase": [
        {
            "args": [("char",)],
            "ret_type_arg_idx": 0
        },
    ],
    "Hypot": [
        {
            "args": [("real", "real")],
            "ret_type_arg_idx": 0
        }
    ],
    "SelectedIntKind": [
        {
            "args": [("int",)],
            "return": "int32"
        }
    ],
    "SelectedRealKind": [
        {
            "args": [("int", "int", "int")],
            "return": "int32"
        }
    ],
    "SelectedCharKind": [
        {
            "args": [("char",)],
            "return": "int32"
        }
    ],
    "Digits": [
        {
            "args": [("int",), ("real",)],
            "return": "int32"
        },
    ],
    "Repeat": [
        {
            "args": [("char", "int")],
            "ret_type_arg_idx": 0
        }
    ],
    "StringContainsSet": [
        {
            "args": [("char", "char", "bool", "int")],
            "ret_type_arg_idx": 3
        }
    ],
    "StringFindSet": [
        {
            "args": [("char", "char", "bool", "int")],
            "ret_type_arg_idx": 3
        }
    ],
    "SubstrIndex": [
        {
            "args": [("char", "char", "bool", "int")],
            "ret_type_arg_idx": 3
        }
    ],
    "MinExponent": [
        {
            "args": [("real",)],
            "return": "int32"
        }
    ],
    "MaxExponent": [
        {
            "args": [("real",)],
            "return": "int32"
        }
    ],
    "Partition": [
        {
            "args": [("char", "char")],
            "ret_type_arg_idx": 0
        }
    ],
    "ListReverse": [
        {
            "args": [("list",)],
            "return": "nullptr"
        }
    ],
    "ListReserve": [
        {
            "args": [("list", "int")],
            "return": "nullptr"
        }
    ],
    "Sign": [
        {
            "args": [("int", "int"), ("real", "real")],
            "ret_type_arg_idx": 0
        },
    ],
    "Radix": [
        {
            "args": [("int",), ("real",)],
            "return": "int32"
        },
    ],
    "Adjustl": [
        {
            "args": [("char",)],
            "return": "character(-1)"
        }
    ],
    "Adjustr": [
        {
            "args": [("char",)],
            "return": "character(-1)"
        }
    ],
    "Aint": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0,
            "kind_arg": True
        }
    ],
    "Nint": [
        {
            "args": [("real",)],
            "return": "int32",
            "kind_arg": True
        }
    ],
    "Anint": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0,
            "kind_arg": True
        }
    ],
    "Floor": [
        {
            "args": [("real",)],
            "return": "int32",
            "kind_arg": True
        }
    ],
    "Ceiling": [
        {
            "args": [("real",)],
            "return": "int32",
            "kind_arg": True
        }
    ],
    "Sqrt": [
        {
            "args": [("real",), ("complex",)],
            "ret_type_arg_idx": 0
        },
    ],
    "Sngl": [
        {
            "args": [("real",)],
            "return": "real32"
        }
    ],
    "SignFromValue": [
        {
            "args": [("int", "int"), ("real", "real")],
            "ret_type_arg_idx": 0
        },
    ],
    "Ifix": [
        {
            "args": [("real",)],
            "return": "int32"
        }
    ],
    "Idint": [
        {
            "args": [("real",)],
            "return": "int32"
        }
    ],
    "Ishft": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Bgt": [
        {
            "args": [("int", "int")],
            "return": "logical"
        },
    ],
    "Blt": [
        {
            "args": [("int", "int")],
            "return": "logical"
        },
    ],
    "Bge": [
        {
            "args": [("int", "int")],
            "return": "logical"
        },
    ],
    "Ble": [
        {
            "args": [("int", "int")],
            "return": "logical"
        },
    ],
    "Lgt": [
        {
            "args": [("char", "char")],
            "return": "logical"
        },
    ],
    "Llt": [
        {
            "args": [("char", "char")],
            "return": "logical"
        },
    ],
    "Lge": [
        {
            "args": [("char", "char")],
            "return": "logical"
        },
    ],
    "Lle": [
        {
            "args": [("char", "char")],
            "return": "logical"
        },
    ],
    "Not": [
        {
            "args": [("int",)],
            "ret_type_arg_idx": 0
        },
    ],
    "Iand": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Ior": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Ieor": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Ibclr": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Ibset": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Btest": [
        {
            "args": [("int", "int")],
            "return": "logical"
        },
    ],
    "Ibits": [
        {
            "args": [("int", "int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Shiftr": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        }
    ],
    "Rshift": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        }
    ],
    "Shiftl": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        }
    ],
    "Aimag": [
        {
            "args": [("complex",)],
            "return": "real32",
            "kind_arg": True
        },
    ],
    "Rank": [
        {
            "args": [("any",)],
            "return": "int32"
        }
    ],
    "Range": [
        {
            "args": [("int",), ("real",), ("complex",)],
            "return": "int32"
        },
    ],
    "Epsilon": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0
        }
    ],
    "Precision": [
        {
            "args": [("real",), ("complex",)],
            "return": "int32"
        }
    ],
    "Tiny": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0
        }
    ],
    "Conjg": [
        {
            "args": [("complex",)],
            "ret_type_arg_idx": 0
        },
    ],
    "Scale": [
        {
            "args": [("real", "int")],
            "ret_type_arg_idx": 0
        }
    ],
    "Huge": [
        {
            "args": [("int",), ("real",)],
            "ret_type_arg_idx": 0
        }
    ],
    "Dprod": [
        {
            "args": [("real", "real")],
            "return": "real64"
        }
    ],
    "Dim": [
        {
            "args": [("int", "int"), ("real", "real")],
            "ret_type_arg_idx": 0
        },
    ],
    "Maskl": [
        {
            "args": [("int",)],
            "return": "int32",
            "kind_arg": True
        }
    ],
    "Maskr": [
        {
            "args": [("int",)],
            "return": "int32",
            "kind_arg": True
        }
    ],
    "Ishftc": [
        {
            "args": [("int", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Ichar": [
        {
            "args": [("char",)],
            "return": "int32",
            "kind_arg": True
        },
    ],
    "Char": [
        {
            "args": [("int",)],
            "return": "character(1)",
            "kind_arg": True
        }
    ],
    "Exponent": [
        {
            "args": [("real",)],
            "return": "int32",
        },
    ],
    "Fraction": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0
        },
    ],
    "SetExponent": [
        {
            "args": [("real", "int")],
            "ret_type_arg_idx": 0
        },
    ],
    "Rrspacing": [
        {
            "args": [("real",)],
            "ret_type_arg_idx": 0
        },
    ],
    "Dshiftl": [
       {
           "args": [("int", "int", "int",)],
           "ret_type_arg_idx": 0
       },
   ],
   "Popcnt": [
       {
           "args": [("int",)],
           "return": "int32",
       },
   ],
   "Poppar": [
       {
           "args": [("int",)],
           "return": "int32",
       },
   ],

}

skip_create_func = ["Partition"]
compile_time_only_fn = [
    "Epsilon",
    "Radix",
    "Range",
    "Precision",
    "Rank",
    "Tiny",
    "Huge",
]

type_to_asr_type_check = {
    "any": "!ASR::is_a<ASR::TypeParameter_t>",
    "int": "is_integer",
    "uint": "is_unsigned_integer",
    "real": "is_real",
    "bool": "is_logical",
    "char": "is_character",
    "complex": "is_complex",
    "dict": "ASR::is_a<ASR::Dict_t>",
    "list": "ASR::is_a<ASR::List_t>",
    "tuple": "ASR::is_a<ASR::Tuple_t>"
}

intrinsic_funcs_ret_type = {
    "Kind": ["int"],
    "Partition": ["tuple"],
    "ListReverse": ["null"],
    "ListReserve": [ "null"],
    "Radix": ["int"],
}

src = ""
indent = "    "

def compute_arg_types(indent, no_of_args, args_arr):
    global src
    for i in range(no_of_args):
        src += indent + f"ASR::ttype_t *arg_type{i} = ASRUtils::type_get_past_const(ASRUtils::expr_type({args_arr}[{i}]));\n"

def compute_arg_condition(no_of_args, args_lists):
    condition = []
    cond_in_msg = []
    for arg_list in args_lists:
        subcond = []
        subcond_in_msg = []
        for i in range(no_of_args):
            arg = arg_list[i]
            subcond.append(f"{type_to_asr_type_check[arg]}(*arg_type{i})")
            subcond_in_msg.append(arg)
        condition.append(" && ".join(subcond))
        cond_in_msg.append(", ".join(subcond_in_msg))
    return (f"({') || ('.join(condition)})", f"({') or ('.join(cond_in_msg)})")

def add_verify_arg_type_src(func_name):
    global src
    arg_infos = intrinsic_funcs_args[func_name]
    no_of_args_msg = ""
    for i, arg_info in enumerate(arg_infos):
        args_lists = arg_info["args"]
        no_of_args = len(args_lists[0])
        no_of_args_msg += " or " if i > 0 else ""
        no_of_args_msg += f"{no_of_args}"
        else_if = "else if" if i > 0 else "if"
        src += 2 * indent + f"{else_if} (x.n_args == {no_of_args}) " + " {\n"
        src += 3 * indent + f'ASRUtils::require_impl(x.m_overload_id == {i}, "Overload Id for {func_name} expected to be {i}, found " + std::to_string(x.m_overload_id), x.base.base.loc, diagnostics);\n'
        compute_arg_types(3 * indent, no_of_args, "x.m_args")
        condition, cond_in_msg = compute_arg_condition(no_of_args, args_lists)
        src += 3 * indent + f'ASRUtils::require_impl({condition}, "Unexpected args, {func_name} expects {cond_in_msg} as arguments", x.base.base.loc, diagnostics);\n'
        src += 2 * indent + "}\n"
    src += 2 * indent + "else {\n"
    src += 3 * indent + f'ASRUtils::require_impl(false, "Unexpected number of args, {func_name} takes {no_of_args_msg} arguments, found " + std::to_string(x.n_args), x.base.base.loc, diagnostics);\n'
    src += 2 * indent + "}\n"

def add_verify_return_type_src(func_name):
    if func_name not in intrinsic_funcs_ret_type.keys():
        return ""
    global src
    ret_type_cond = ""
    ret_type_cond_in_msg = ""
    for i, ret_type in enumerate(intrinsic_funcs_ret_type[func_name]):
        if ret_type == "null":
            ret_type_cond += f"x.m_type == nullptr"
        else:
            ret_type_cond += f"{type_to_asr_type_check[ret_type]}(*x.m_type)"
        ret_type_cond_in_msg += f"{ret_type}"
        if i < len(intrinsic_funcs_ret_type[func_name]) - 1:
            ret_type_cond += " || "
            ret_type_cond_in_msg += " or "
    src += 2 * indent + f'ASRUtils::require_impl({ret_type_cond}, "Unexpected return type, {func_name} expects `{ret_type_cond_in_msg}` as return type", x.base.base.loc, diagnostics);\n'

def add_create_func_arg_type_src(func_name):
    global src
    arg_infos = intrinsic_funcs_args[func_name]
    no_of_args_msg = ""
    for i, arg_info in enumerate(arg_infos):
        args_lists = arg_info["args"]
        kind_arg = arg_info.get("kind_arg", False)
        no_of_args = len(args_lists[0])
        no_of_args_msg += " or " if i > 0 else ""
        no_of_args_msg += f"{no_of_args + int(kind_arg)}"
        else_if = "else if" if i > 0 else "if"
        src += 2 * indent + f"{else_if} (args.size() == {no_of_args + int(kind_arg)}) " + " {\n"
        compute_arg_types(3 * indent, no_of_args, "args")
        condition, cond_in_msg = compute_arg_condition(no_of_args, args_lists)
        src += 3 * indent + f'if(!({condition}))' + ' {\n'
        src += 4 * indent + f'append_error(diag, "Unexpected args, {func_name} expects {cond_in_msg} as arguments", loc);\n'
        src += 4 * indent + f'return nullptr;\n'
        src += 3 * indent + '}\n'
        src += 2 * indent + "}\n"
    src += 2 * indent + "else {\n"
    src += 3 * indent + f'append_error(diag, "Unexpected number of args, {func_name} takes {no_of_args_msg} arguments, found " + std::to_string(args.size()), loc);\n'
    src += 3 * indent + f'return nullptr;\n'
    src += 2 * indent + "}\n"


def add_create_func_return_src(func_name):
    global src, indent
    arg_infos = intrinsic_funcs_args[func_name]
    args_lists = arg_infos[0]["args"]
    no_of_args = len(args_lists[0])
    ret_type_val = arg_infos[0].get("return", None)
    ret_type_arg_idx = arg_infos[0].get("ret_type_arg_idx", None)
    if ret_type_val:
        ret_type = ret_type_val
    else:
        src += indent * 2 + "ASRUtils::ExprStmtDuplicator expr_duplicator(al);\n"
        src += indent * 2 + "expr_duplicator.allow_procedure_calls = true;\n"
        src += indent * 2 + f"ASR::ttype_t* type_ = expr_duplicator.duplicate_ttype(expr_type(args[{ret_type_arg_idx}]));\n"
        ret_type = "type_"
    kind_arg = arg_infos[0].get("kind_arg", False)
    src += indent * 2 + f"ASR::ttype_t *return_type = {ret_type};\n"
    if kind_arg:
        src += indent * 2 + "if ( args[1] != nullptr ) {\n"
        src += indent * 3 +     "int kind = -1;\n"
        src += indent * 3 +     "if (!ASR::is_a<ASR::Integer_t>(*expr_type(args[1])) || !extract_value(args[1], kind)) {\n"
        src += indent * 4 +         f'append_error(diag, "`kind` argument of the `{func_name}` function must be a scalar Integer constant", args[1]->base.loc);\n'
        src += indent * 4 +         "return nullptr;\n"
        src += indent * 3 +     "}\n"
        src += indent * 3 +     "set_kind_to_ttype_t(return_type, kind);\n"
        src += indent * 2 + "}\n"
    src += indent * 2 + "ASR::expr_t *m_value = nullptr;\n"
    src += indent * 2 + f"Vec<ASR::expr_t*> m_args; m_args.reserve(al, {no_of_args});\n"
    for _i in range(no_of_args):
        src += indent * 2 + f"m_args.push_back(al, args[{_i}]);\n"
    if func_name in compile_time_only_fn:
        src += indent * 2 + f"return_type = ASRUtils::extract_type(return_type);\n"
        src += indent * 2 + f"m_value = eval_{func_name}(al, loc, return_type, args, diag);\n"
        src += indent * 2 + "return ASR::make_TypeInquiry_t(al, loc, "\
            f"static_cast<int64_t>(IntrinsicElementalFunctions::{func_name}), "\
            "ASRUtils::expr_type(m_args[0]), m_args[0], return_type, m_value);\n"

    else:
        src += indent * 2 + "if (all_args_evaluated(m_args)) {\n"
        src += indent * 3 +     f"Vec<ASR::expr_t*> args_values; args_values.reserve(al, {no_of_args});\n"
        for _i in range(no_of_args):
            src += indent * 3 + f"args_values.push_back(al, expr_value(m_args[{_i}]));\n"
        src += indent * 3 +     f"m_value = eval_{func_name}(al, loc, return_type, args_values, diag);\n"
        src += indent * 2 + "}\n"
        if "null" in intrinsic_funcs_ret_type.get(func_name, []):
            src += indent * 2 + f"return ASR::make_Expr_t(al, loc, ASRUtils::EXPR(ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::{func_name}), m_args.p, m_args.n, 0, return_type, m_value)));\n"
        else:
            src += indent * 2 + f"return ASR::make_IntrinsicElementalFunction_t(al, loc, static_cast<int64_t>(IntrinsicElementalFunctions::{func_name}), m_args.p, m_args.n, 0, return_type, m_value);\n"

def gen_verify_args(func_name):
    global src
    src += indent + R"static inline void verify_args(const ASR::IntrinsicElementalFunction_t& x, diag::Diagnostics& diagnostics) {" + "\n"
    add_verify_arg_type_src(func_name)
    if func_name in compile_time_only_fn:
        src += indent * 2 + 'ASRUtils::require_impl(x.m_value, '\
            f'"Missing compile time value, `{func_name}` intrinsic output must '\
            'be computed during compile time", x.base.base.loc, diagnostics);\n'
    add_verify_return_type_src(func_name)
    src += indent + "}\n\n"

def gen_create_function(func_name):
    global src
    src += indent + Rf"static inline ASR::asr_t* create_{func_name}(Allocator& al, const Location& loc, Vec<ASR::expr_t*>& args, diag::Diagnostics& diag) " + "{\n"
    add_create_func_arg_type_src(func_name)
    add_create_func_return_src(func_name)
    src += indent + "}\n"


def get_registry_funcs_src():
    global src
    for func_name in intrinsic_funcs_args.keys():
        src += f"namespace {func_name}" + " {\n\n"
        gen_verify_args(func_name)

        if func_name not in skip_create_func:
            gen_create_function(func_name)
        src += "}\n\n"
    return src


HEAD = """#ifndef LIBASR_PASS_INTRINSIC_FUNC_REG_UTIL_H
#define LIBASR_PASS_INTRINSIC_FUNC_REG_UTIL_H

#include <libasr/pass/intrinsic_functions.h>

namespace LCompilers {

namespace ASRUtils {

"""

FOOT = """
} // namespace ASRUtil

} // namespace LCompilers

#endif // LIBASR_PASS_INTRINSIC_FUNC_REG_UTIL_H
"""
def main(argv):
    if len(argv) == 2:
        out_file = argv[1]
    elif len(argv) == 1:
        print("Assuming default values of intrinsic_function_registry_util.h")
        here = os.path.dirname(__file__)
        pass_dir = os.path.join(here, "pass")
        out_file = os.path.join(pass_dir, "intrinsic_function_registry_util.h")
    else:
        print("invalid arguments")
        return 2
    fp = open(out_file, "w", encoding="utf-8")
    try:
        fp.write(HEAD)
        fp.write(get_registry_funcs_src())
        fp.write(FOOT)
    finally:
        fp.close()

if __name__ == "__main__":
    sys.exit(main(sys.argv))
