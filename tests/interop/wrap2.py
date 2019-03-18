"""
# Design

This file converts from a GFortran module file representation (documented in
the `lfortran.adapters.gfortran.mod` module) to an Abstract Semantic Representation (ASR).
"""

# TODO: move this into the lfortran package itself
import sys
sys.path.append("../..")

from lfortran.asr import asr
from lfortran.asr.asr_check import verify_asr
from lfortran.asr.asr_to_ast import asr_to_ast
from lfortran.ast.ast_to_src import ast_to_src
from lfortran.adapters.gfortran.mod import mod_to_asr
from lfortran.asr.builder import (make_translation_unit,
    translation_unit_make_module, scope_add_function, make_type_integer,
    make_type_real, type_eq, make_binop, scope_add_symbol, Scope,
    function_make_var, array_is_assumed_shape)

class NodeTransformer(asr.NodeTransformerBase):

    def visit_scope(self, symtab, parent=None):
        new_symtab = Scope(parent)
        self._lookup = 2
        self._scope = new_symtab
        for s, sym in symtab.symbols.items():
            new_symtab.symbols[s] = self.visit(sym)
        self._lookup = 0
        return new_symtab

    def visit_sequence(self, seq):
        r = []
        if seq is not None:
            for node in seq:
                r.append(self.visit(node))
        return r

    def visit_object(self, o):
        if isinstance(o, Scope):
            return self.visit_scope(o)
        elif isinstance(o, list):
            return self.visit_sequence(o)
        elif isinstance(o, (str, int)) or o is None:
            return o
        else:
            print(type(o))
            raise NotImplementedError()

    def visit_Function(self, node):
        name = self.visit_object(node.name)
        symtab = self.visit_object(node.symtab)
        self._lookup = 1
        self._scope = symtab
        args = self.visit_sequence(node.args)
        body = self.visit_sequence(node.body)
        return_var = self.visit(node.return_var)
        self._lookup = 0
        if node.bind:
            bind = self.visit(node.bind)
        else:
            bind = None
        return asr.Function(name=name, args=args, body=body, bind=bind, return_var=return_var, symtab=symtab)

    def visit_Variable(self, node):
        if self._lookup == 1:
            return self._scope.resolve(node.name)
        elif self._lookup == 2:
            v = self._scope.resolve(node.name, raise_exception=False)
            if v:
                return v
        name = self.visit_object(node.name)
        intent = self.visit_object(node.intent)
        dummy = self.visit_object(node.dummy)
        type = self.visit(node.type)
        return asr.Variable(name=name, intent=intent, dummy=dummy, type=type)


class WrapperVisitor(NodeTransformer):

    def visit_Module(self, node):
        name = node.name + "_wrapper"
        self._modname = node.name
        symtab = self.visit_scope(node.symtab)
        return asr.Module(name=name, symtab=symtab)

    def visit_Function(self, node):
        name = self.visit_object(node.name)
        symtab = self.visit_object(node.symtab)
        self._lookup = 1
        self._scope = symtab
        args = self.visit_sequence(node.args)
        body = self.visit_sequence(node.body)
        self._lookup = 0
        if node.bind:
            bind = self.visit(node.bind)
        else:
            bind = None
        tmp = asr.Variable(name="a", type=make_type_integer())
        f = asr.Function(name=name, args=args, body=body, bind=bind,
            symtab=symtab, return_var=tmp)
        return_var = function_make_var(f, name="r", type=self.visit(node.return_var.type))
        return_var.dummy = True
        f.return_var = return_var

        cname = node.name + "_c_wrapper"
        mangled_name = '__' + self._modname + '_MOD_' + node.name.lower()
        bind = asr.Bind(lang="c", name=mangled_name)
        cargs = []
        for arg in node.args:
            type = self.visit(arg.type)
            if array_is_assumed_shape(type):
                if len(type.dims) == 1:
                    dname = "c_desc1_t"
                elif len(type.dims) == 2:
                    dname = "c_desc2_t"
                else:
                    raise NotImplementedError("Too many dimensions")
                type = asr.Derived(name=dname)
            cargs.append(asr.Variable(
                name=arg.name,
                intent=arg.intent,
                type=type,
            ))
        fw = scope_add_function(symtab, cname, args=cargs, return_var=cname)
        fw.bind = bind

        body = [
            asr.Assignment(return_var,
                asr.FuncCall(func=fw, args=args, keywords=[],
                    type=fw.return_var.type)
            )
        ]
        f.body = body

        return f

def create_wrapper(u):
    v = WrapperVisitor()
    u2 = v.visit(u)
    verify_asr(u2)
    return u2

u = mod_to_asr("mod1.mod")
a = asr_to_ast(u)
s = ast_to_src(a)
print(s)

u2 = create_wrapper(u)
a = asr_to_ast(u2)
s = ast_to_src(a)
print("-"*80)
print(s)
