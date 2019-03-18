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
    make_type_real, type_eq, make_binop, scope_add_symbol, Scope)

class NodeTransformer(asr.NodeTransformerBase):

    def visit_scope(self, symtab, parent=None):
        new_symtab = Scope(parent)
        for s, sym in symtab.symbols.items():
            new_symtab.symbols[s] = self.visit(sym)
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
        # As a workaround: we need to rebuild Function with the new symbol
        # table
        return node

class WrapperVisitor(NodeTransformer):

    def visit_Module(self, node):
        name = node.name + "_wrapper"
        symtab = self.visit_scope(node.symtab)
        return asr.Module(name=name, symtab=symtab)

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
