"""
# ASR Builder

Using the ASR builder has the following advantages over constructing the ASR
directly:

* The ASR is constructed correctly, or a nice error is given
* Is easier to use, for example it handles the scoped symbol table
  automatically
* The API is natural to be used in the semantic analyzer, making its AST
  visitor as simple as possible (by handling many things automatically)

## TODO

* Add asr.FunctionType: The FunctionType will not have a symbol table, but will
  have a list of arguments as Variables. FunctionType has a name associated
  with it --- this will either be the name used in the `procedure(some_name)`
  declaration, or as a function name if used in Function(..).

* Rename FunctionBuilder() to FunctionTypeBuilder(), and once build, the
  symbol table will have an asr.FunctionType() instance.

* the asr.Function will just be asr.Function(FunctionType(), Body()), the name
  of the function is stored in FunctionType.

* asr.Function will have a symbol table, but asr.FunctionSignature() will not

* Add a VariableBuilder to construct Variable field by field, and it's
  VariableBuilder.finalize() method will be called from
  FunctionTypeBuilder.finalize()

Function call will simply reference the symbol table which has a FunctionType
symbol.

## Immutability

All ASR classes are mutable when constructing them. One is free to create just
asr.Variable() and populate it later. Then one calls "verify_asr()", everything
gets checked and error reported. After that, all classes are immutable.

"""

from . import asr
from .asr_check import verify_asr
from ..semantic.analyze import Scope


def scope_add_symbol(scope, v):
    scope.symbols[v.name] = v
    v.scope = scope

def make_type_integer(kind=None):
    if not kind:
        kind = 4
    return asr.Integer(kind=kind)

def make_type_real(kind=None):
    if not kind:
        kind = 8
    return asr.Real(kind=kind)

def make_translation_unit():
    return asr.TranslationUnit(global_scope=Scope())

def translation_unit_make_module(unit, name):
    module_scope = Scope(unit.global_scope)
    m = asr.Module(name=name, symtab=module_scope)
    scope_add_symbol(unit.global_scope, m)
    return m

def make_function(mod, name, args=[], return_var=None, body=None):
    assert isinstance(mod, asr.Module)
    parent_scope = mod.symtab
    function_scope = Scope(parent_scope)
    args_ = []
    for arg in args:
        if isinstance(arg, str):
            arg = asr.Variable(name=arg, type=make_type_integer())
        arg.dummy = True
        scope_add_symbol(function_scope, arg)
        args_.append(arg)
    if return_var:
        if isinstance(return_var, str):
            return_var = asr.Variable(name=return_var, type=make_type_integer())
        return_var.dummy = True
        scope_add_symbol(function_scope, return_var)
    if body is None:
        # TODO: body=None should mean no body, and body=[] an empty body
        body = []
    f = asr.Function(name=name, symtab=function_scope,
            args=args_, return_var=return_var, body=body)
    scope_add_symbol(parent_scope, f)
    return f

def function_make_var(fn, name, type):
    v = asr.Variable(name=name, dummy=False, type=type)
    scope_add_symbol(fn.symtab, v)
    return v