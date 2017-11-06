"""
Semantic analysis module
"""

from ..ast import ast

class Type(object):
    def __init__(self, varname):
        self.varname = varname

class Intrinsic(Type):
    def __init__(self, varname, kind=None):
        super(Intrinsic, self).__init__(varname)
        self.kind = kind

class Integer(Intrinsic): pass
class Real(Intrinsic): pass
class Complex(Intrinsic): pass
class Character(Intrinsic): pass
class Logical(Intrinsic): pass

class Derived(Type):
    pass

class Array(Type):
    def __init__(self, varname, type_, rank, shape):
        super(Array, self).__init__(varname)
        self.type_ = type_
        self.rank = rank
        self.shape = shape

class SymbolTableVisitor(ast.GenericASTVisitor):

    def __init__(self):
        self.types = {
                "integer": Integer,
                "real": Real,
                "complex": Complex,
                "character": Character,
                "logical": Logical,
            }
        self.symbol_table = {}


    def visit_Declaration(self, node):
        for v in node.vars:
            sym = v.sym
            type_f = v.sym_type
            if type_f not in self.types:
                raise Exception("Type not implemented.")
            type_ = self.types[type_f](sym)
            self.symbol_table[sym] = type_
