"""
Install Python wrappers to Clang:

    conda install -c conda-forge python-clang
"""
from clang.cindex import Index, CursorKind, TypeKind

import sys
sys.path.append("../..")
from lfortran.asr import asr
from lfortran.asr.asr_check import verify_asr
from lfortran.asr.asr_to_ast import asr_to_ast
from lfortran.ast.ast_to_src import ast_to_src
from lfortran.asr.builder import (make_translation_unit,
    translation_unit_make_module, scope_add_function, make_type_integer,
    make_type_real, type_eq, make_binop, scope_add_symbol, Scope,
    function_make_var, array_is_assumed_shape)

index = Index.create()
tu = index.parse("a.h")

def process(node):
    unit = make_translation_unit()
    module = translation_unit_make_module(unit, "cxx_wrapper")
    for child in node.get_children():
        if child.kind == CursorKind.FUNCTION_DECL:
            process_function(module.symtab, child)
        else:
            print(child.kind.name, child.spelling, child.location)
            #import IPython; IPython.embed()
    verify_asr(unit)
    return unit

def process_type(t):
    if t.kind == TypeKind.FUNCTIONPROTO:
        result = process_type(t.get_result())
        args = []
        for arg in t.argument_types():
            args.append(process_type(arg))
        return "result(%s), args(%s)" % (result, ", ".join(args))
    elif t.kind == TypeKind.POINTER:
        pointee = process_type(t.get_pointee())
        return "pointer(%s)" % pointee
    elif t.kind == TypeKind.INT:
        return "int"
    else:
        print(t.spelling)
        raise Exception("Type not supported")

def process_function(scope, node):
    types = process_type(node.type)
    arg_names = []
    for arg in node.get_arguments():
        arg_names.append(arg.spelling)
    type = make_type_integer()
    return_var = asr.Variable(name=node.spelling, type=type)
    f = scope_add_function(scope, node.spelling, args=arg_names,
            return_var=return_var)
    for a in f.args:
        a.intent = "in"
    f.bind = asr.Bind(lang="c", name=node.spelling)

u = process(tu.cursor)
a = asr_to_ast(u)
s = ast_to_src(a)
print(s)
