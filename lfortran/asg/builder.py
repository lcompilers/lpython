"""
# ASG Builder

Using the ASG builder has the following advantages over constructing the ASG
directly:

* The ASG is constructed correctly, or a nice error is given
* Is easier to use, for example it handles the scoped symbol table
  automatically

"""

from . import asg
from .asg_check import check_function
from ..semantic.analyze import Scope

# Private:

def _add_symbol(scope, v):
    scope.symbols[v.name] = v
    v._scope = scope

def _add_var(scope, v, dummy=False):
    v.scope = scope
    v.dummy = dummy
    _add_symbol(scope, v)

# Public API:

def make_type_integer(kind=None):
    if not kind:
        kind = 4
    return asg.Integer(kind=kind)


class FunctionBuilder():

    def __init__(self, mod, name, args=[], return_var=None, body=[]):
        assert isinstance(mod, asg.Module)
        scope = mod.symtab
        self._name = name
        self._args = args.copy()
        self._body = body.copy()
        self._return_var = return_var
        self._parent_scope = scope
        self._function_scope = Scope(self._parent_scope)
        for arg in args:
            _add_var(self._function_scope, arg, dummy=True)
        if return_var:
            _add_var(self._function_scope, return_var, dummy=True)

    def make_var(self, name, type):
        v = asg.Variable(name=name, dummy=False, type=type)
        _add_var(self._function_scope, v, dummy=False)
        return v

    def add_statements(self, statements):
        assert isinstance(statements, list)
        self._body += statements

    def finalize(self):
        f = asg.Function(name=self._name, symtab=self._function_scope,
                args=self._args, return_var=self._return_var, body=self._body)
        _add_symbol(self._parent_scope, f)
        check_function(f)
        return f

class TranslationUnit():

    def __init__(self):
        self._global_scope = Scope()
        pass

    def make_module(self, name):
        module_scope = Scope(self._global_scope)
        m = asg.Module(name=name, symtab=module_scope)
        _add_symbol(self._global_scope, m)
        return m
