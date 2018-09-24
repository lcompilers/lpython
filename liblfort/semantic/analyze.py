"""
Semantic analysis module
"""

from ..ast import ast

class SemanticError(Exception):
    pass

class UndeclaredVariableError(SemanticError):
    pass

class TypeMismatch(SemanticError):
    pass


# ----------------------------

# Types are immutable. Once instantiated, they never change.

class Type(object):

    def __neq__(self, other):
        return not self.__eq__(other)

class Intrinsic(Type):
    def __init__(self, kind=None):
        #super(Intrinsic, self).__init__()
        self.kind = kind

    def __eq__(self, other):
        if type(self) != type(other):
            return False
        return self.kind == other.kind

    def __hash__(self):
        return 1

class Integer(Intrinsic):
    def __repr__(self):
        return "Integer()"
class Real(Intrinsic):
    def __repr__(self):
        return "Real()"
class Complex(Intrinsic): pass
class Character(Intrinsic): pass
class Logical(Intrinsic): pass

class Derived(Type):
    pass


class Array(Type):

    def __init__(self, type_, shape):
        self.type_ = type_
        self.shape = shape

    def __eq__(self, other):
        if type(self) != type(other):
            return False
        return self.type_ == other.type_ and self.shape == other.shape

    def __hash__(self):
        return 1

    def __repr__(self):
        return "Array(%r, %r)" % (self.type_, self.shape)


# --------------------------------

class Scope:

    def __init__(self, parent_scope=None):
        self._parent_scope = parent_scope
        self._local_symbols = {}

    @property
    def parent_scope(self):
        return self._parent_scope

    def resolve(self, sym):
        if sym in self._local_symbols:
            return self._local_symbols[sym]
        elif self._parent_scope:
            return self._parent_scope.resolve(sym)
        else:
            # Symbol not found
            return None


class SymbolTableVisitor(ast.GenericASTVisitor):

    def __init__(self):
        self.types = {
                "integer": Integer,
                "real": Real,
                "complex": Complex,
                "character": Character,
                "logical": Logical,
            }
        self.__symbol_table = { x: {
                "name": x,
                "type": Real(),
                "external": True,
                "func": True,
                "global": True,
            } for x in [
                "abs",
                "sqrt",
                "log",
                "sum",
                "random_number",
            ]}
        self._global_level = True

        self._global_scope = Scope()
        self._current_scope = self._global_scope

        # Temporary:
        self._global_scope._old_symbol_table = self.__symbol_table

    def mark_all_external(self):
        """
        Marks all symbols in the symbol table as external.
        """
        for sym in self.__symbol_table:
            self.__symbol_table[sym]["external"] = True

    def visit_Declaration(self, node):
        for v in node.vars:
            sym = v.sym
            type_f = v.sym_type
            if type_f not in self.types:
                # This shouldn't happen, as the parser checks types
                raise SemanticError("Type not implemented.")
            dims = []
            for dim in v.dims:
                if isinstance(dim.end, ast.Num):
                    dims.append(int(dim.end.n))
                else:
                    raise NotImplementedError("Index not a number")
                if dim.start:
                    raise NotImplementedError("start index")
            type_ = self.types[type_f]()
            if len(dims) > 0:
                type_ = Array(type_, dims)
            sym_data = {"name": sym, "type": type_,
                "global": self._global_level, "external": False, "func": False}
            self.__symbol_table[sym] = sym_data
            self._current_scope._local_symbols[sym] = sym_data

    def visit_Function(self, node):
        sym = node.name
        # TODO: for now we assume integer result, but we should read the AST
        # and determine the type of the result.
        type_ = self.types["integer"]()
        self.__symbol_table[sym] = {
                "name": sym,
                "type": type_,
                "external": False,
                "global": self._global_level,
                "func": True,
            }

        node._scope = Scope(self._current_scope)
        self._current_scope = node._scope

        gl = self._global_level
        self._global_level = False
        # TODO: put these declarations into the scoped symbol table for this
        # function only:
        self.visit_sequence(node.decl)

        # Iterate over nested functions
        self.visit_sequence(node.contains)
        self._global_level = gl

        self._current_scope = node._scope.parent_scope

    def visit_Program(self, node):
        node._scope = Scope(self._current_scope)
        self._current_scope = node._scope

        gl = self._global_level
        self._global_level = False
        self.visit_sequence(node.decl)
        #self.visit_sequence(node.body)
        self.visit_sequence(node.contains)
        self._global_level = gl

        self._current_scope = node._scope.parent_scope

    def visit_Subroutine(self, node):
        node._scope = Scope(self._current_scope)
        self._current_scope = node._scope

        gl = self._global_level
        self._global_level = False
        self.visit_sequence(node.args)
        self.visit_sequence(node.decl)
        #self.visit_sequence(node.body)
        self.visit_sequence(node.contains)
        self._global_level = gl

        self._current_scope = node._scope.parent_scope

def create_symbol_table(tree):
    v = SymbolTableVisitor()
    v.visit(tree)
    return v._global_scope



class ExprVisitor(ast.GenericASTVisitor):

    def __init__(self, symbol_table, global_scope):
        self.symbol_table = symbol_table
        self._current_scope = global_scope

    def visit_Program(self, node):
        self._current_scope = node._scope
        self.visit_sequence(node.decl)
        self.visit_sequence(node.body)
        self.visit_sequence(node.contains)
        self._current_scope = node._scope.parent_scope

    def visit_Subroutine(self, node):
        self._current_scope = node._scope
        self.visit_sequence(node.args)
        self.visit_sequence(node.decl)
        self.visit_sequence(node.body)
        self.visit_sequence(node.contains)
        self._current_scope = node._scope.parent_scope

    def visit_Function(self, node):
        self._current_scope = node._scope
        self.visit_sequence(node.args)
        self.visit_sequence(node.decl)
        self.visit_sequence(node.body)
        self.visit_sequence(node.contains)
        self._current_scope = node._scope.parent_scope

    def visit_Num(self, node):
        try:
            node.o = int(node.n)
            node._type = Integer()
        except ValueError:
            try:
                node.o = float(node.n)
                node._type = Real()
            except ValueError:
                if node.n.endswith("_dp"):
                    node.o = float(node.n[:-3])
                    node._type = Real()
                else:
                    raise SemanticError("Unknown number syntax.")

    def visit_Constant(self, node):
        node._type = Logical()

    def visit_Name(self, node):
        if not node.id in self.symbol_table:
            raise UndeclaredVariableError("Variable '%s' not declared." \
                    % node.id)
        #node._type = self.symbol_table[node.id]["type"]
        print(self._current_scope.resolve(node.id))
        node._type = self._current_scope.resolve(node.id)["type"]

    def visit_FuncCallOrArray(self, node):
        if not node.func in self.symbol_table:
            raise UndeclaredVariableError("Func or Array '%s' not declared." \
                    % node.func)
        self.visit_sequence(node.args)
        node._type = self.symbol_table[node.func]["type"]

    def visit_BinOp(self, node):
        self.visit(node.left)
        self.visit(node.right)
        if node.left._type == node.right._type:
            node._type = node.left._type
        else:
            if isinstance(node.left._type, Array) and \
                    isinstance(node.right._type, Array):
                if node.left._type.type_ != node.right._type.type_:
                    raise TypeMismatch("Array Type mismatch")
                node._type = node.right._type.type_
            elif isinstance(node.left._type, Array):
                if node.left._type.type_ != node.right._type:
                    raise TypeMismatch("Array Type mismatch")
                node._type = node.right._type
            elif isinstance(node.right._type, Array):
                if node.right._type.type_ != node.left._type:
                    raise TypeMismatch("Array Type mismatch")
                node._type = node.left._type
            else:
                # TODO: allow combinations of Real/Integer
                raise TypeMismatch("Type mismatch")

    def visit_UnaryOp(self, node):
        self.visit(node.operand)
        node._type = node.operand._type

    def visit_Compare(self, node):
        self.visit(node.left)
        self.visit(node.right)
        if node.left._type != node.right._type:
            if isinstance(node.left._type, Array):
                if node.left._type.type_ != node.right._type:
                    raise TypeMismatch("Array Type mismatch")
            elif isinstance(node.right._type, Array):
                if node.right._type.type_ != node.left._type:
                    raise TypeMismatch("Array Type mismatch")
            else:
                # TODO: allow combinations of Real/Integer
                raise TypeMismatch("Type mismatch")
        node._type = Logical()

    def visit_If(self, node):
        self.visit(node.test)
        if node.test._type != Logical():
            raise TypeMismatch("If condition must be of type logical.")
        self.visit_sequence(node.body)
        self.visit_sequence(node.orelse)

    def visit_Assignment(self, node):
        if isinstance(node.target, ast.Name):
            if not node.target.id in self.symbol_table:
                raise UndeclaredVariableError("Variable '%s' not declared." \
                        % node.target)
            node._type = self.symbol_table[node.target.id]["type"]
            self.visit(node.value)
            if node.value._type != node._type:
                if isinstance(node._type, Array):
                    # FIXME: this shouldn't happen --- this is handled by the
                    # next if
                    if node._type.type_ != node.value._type:
                        raise TypeMismatch("LHS Array Type mismatch")
                if isinstance(node.value._type, Array):
                    if node._type != node.value._type.type_:
                        raise TypeMismatch("RHS Array Type mismatch")
                else:
                    raise TypeMismatch("Type mismatch")
        elif isinstance(node.target, ast.FuncCallOrArray):
            if not node.target.func in self.symbol_table:
                raise UndeclaredVariableError("Array '%s' not declared." \
                        % node.target.func)
            node._type = self.symbol_table[node.target.func]["type"]
            # TODO: based on symbol_table, figure out if `node.target` is an
            # array or a function call. Annotate the `node` to reflect that.
            # Then in later stage the node can be replaced, based on the
            # annotation. Extend Fortran.asdl to include FuncCall and
            # ArrayRef, and change all occurrences of FuncCallOrArray to one of
            # those.
            # For now we assume that FuncCallOrArray is an ArrayRef.
            self.visit_sequence(node.target.args)
            if len(node.target.args) != 1:
                raise NotImplementedError("Require exactly one index for now")
            idx = node.target.args[0]
            if idx._type != Integer():
                raise TypeMismatch("Array index must be an integer")
            self.visit(node.value)
            if node.value._type != node._type:
                if isinstance(node._type, Array):
                    if isinstance(node.value._type, Array):
                        if node._type.type_ != node.value._type.type_:
                            raise TypeMismatch("Array Type mismatch")
                    else:
                        if node._type.type_ != node.value._type:
                            raise TypeMismatch("Array Type mismatch")
                else:
                    # FIXME: this shouldn't happen --- this is handled by the
                    # above if
                    raise TypeMismatch("Type mismatch")
        else:
            raise SemanticError("LHS must be a variable or an array")

def annotate_tree(tree, global_scope):
    """
    Annotates the `tree` with types.
    """
    symbol_table = global_scope._old_symbol_table
    v = ExprVisitor(symbol_table, global_scope)
    v.visit(tree)