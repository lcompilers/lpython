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

class WrapperVisitor(asr.ASTVisitor):

    def visit_TranslationUnit(self, node):
        out = ""
        for s in node.global_scope.symbols:
            sym = node.global_scope.symbols[s]
            out += self.visit(sym)
        return out

    def visit_Module(self, node):
        name = "mod2"
        out = """\
#ifndef MOD2_H
#define MOD2_H

extern "C" {

"""
        out += self.visit_scope(node.symtab)
        out += """\
}

#endif // MOD2_H
"""
        return out

    def visit_scope(self, symtab, parent=None):
        out = ""
        for s, sym in symtab.symbols.items():
            out += self.visit(sym)
        return out

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
        args2 = []
        for arg in node.args:
            #type = self.visit(arg.type)
            #if array_is_assumed_shape(type):
            #    if len(type.dims) == 1:
            #        dname = "c_desc1_t"
            #    elif len(type.dims) == 2:
            #        dname = "c_desc2_t"
            #    else:
            #        raise NotImplementedError("Too many dimensions")
            #    args2.append(dname)
            #else:
            #    args2.append(arg)
            n = len(arg.type.dims)
            if n == 0:
                args2.append("int32_t *%s" % arg.name)
            else:
                if array_is_assumed_shape(arg.type):
                    args2.append("descriptor<%s, int32_t> *%s" % (n, arg.name))
                else:
                    args2.append("int32_t *%s" % arg.name)
        #symtab = self.visit_object(node.symtab)
        #self._lookup = 1
        #self._scope = symtab
        #args = self.visit_sequence(node.args)
        #body = self.visit_sequence(node.body)
        #return_var = self.visit(node.return_var)
        #self._lookup = 0
        #if node.bind:
        #    bind = self.visit(node.bind)
        #else:
        #    bind = None
        return "int32_t __mod1_MOD_%s(%s);\n" % (name, ", ".join(args2))
        #return asr.Function(name=name, args=args, body=body, bind=bind, return_var=return_var, symtab=symtab)



def create_wrapper(u):
    v = WrapperVisitor()
    u2 = v.visit(u)
    return u2

u = mod_to_asr("mod1.mod")
u2 = create_wrapper(u)
print(u2)
