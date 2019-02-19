"""
Semantic analysis module.

It declares Fortran types as Python classes. It contains a visitor pattern to
create a symbol table (SymbolTableVisitor) that is run first. Then it
contains another visitor pattern that walks all expressions and assigns/check
types based on the symbol table (ExprVisitor) that is run second.
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
class Complex(Intrinsic):
    def __repr__(self):
        return "Complex()"
class Character(Intrinsic):
    def __repr__(self):
        return "Character()"
class Logical(Intrinsic):
    def __repr__(self):
        return "Logical()"

class Derived(Type):
    def __repr__(self):
        return "Derived()"


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
    """
    A Scope contains local symbols and a link to the parent scope.

    It is attached directly to the AST nodes for functions, subroutines and
    programs. When using a visitor pattern, one must propagate the "current
    scope" by hand, so that one can access the function's scope from, say,
    the assignment statement.
    """

    def __init__(self, parent_scope=None):
        self._parent_scope = parent_scope
        self._local_symbols = {}

    @property
    def parent_scope(self):
        return self._parent_scope

    @property
    def symbols(self):
        return self._local_symbols

    def resolve(self, sym, raise_exception=True):
        if sym in self._local_symbols:
            return self._local_symbols[sym]
        elif self._parent_scope:
            return self._parent_scope.resolve(sym, raise_exception)
        else:
            # Symbol not found
            if raise_exception:
                # This only happens when there is a bug. The semantic phase
                # should call resolve() with raise_exception=False and create
                # nice user errors. The code generation phase calls resolve()
                # with raise_exception=True.
                raise Exception("Internal error: Symbol '%s' not declared." \
                        % sym)
            else:
                return None


class SymbolTableVisitor(ast.GenericASTVisitor):

    def __init__(self):
        # Mapping between Fortran type strings and LFortran type classes
        self.types = {
                "integer": Integer,
                "real": Real,
                "complex": Complex,
                "character": Character,
                "logical": Logical,
            }

        self._global_scope = Scope()
        self._global_scope._local_symbols = { x: {
                "name": x,
                "type": Real(),
                "external": True,
                "func": True,
                "subroutine": False,
                "global": True,
            } for x in [
                "abs",
                "sqrt",
                "log",
                "sum",
                "random_number",
            ]}

        self._global_scope._local_symbols["plot"] = {
            "name": "plot",
            "type": Integer(),
            "external": True,
            "func": True, # Function or Subroutine
            "global": True,
        }
        self._global_scope._local_symbols["savefig"] = {
            "name": "savefig",
            "type": Integer(),
            "external": True,
            "func": True, # Function or Subroutine
            "global": True,
        }
        self._global_scope._local_symbols["show"] = {
            "name": "show",
            "type": Integer(),
            "external": True,
            "func": True, # Function or Subroutine
            "global": True,
        }
        self._global_scope._local_symbols["plot_test"] = {
            "name": "plot_test",
            "type": Integer(),
            "external": True,
            "func": True, # Function or Subroutine
            "global": True,
        }

        self._current_scope = self._global_scope

    def mark_all_external(self):
        """
        Marks all symbols in the symbol table as external.
        """
        for sym in self._global_scope.symbols:
            self._global_scope.symbols[sym]["external"] = True

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
                "external": False, "func": False, "dummy": False,
                "intent": None}
            for a in v.attrs:
                if a.name == "intent":
                    sym_data["intent"] = a.args[0].arg
            self._current_scope._local_symbols[sym] = sym_data

    def visit_Function(self, node):
        sym = node.name
        # TODO: for now we assume integer result, but we should read the AST
        # and determine the type of the result.
        type_ = self.types["integer"]()

        sym_data = {
                "name": sym,
                "type": type_,
                "args": node.args,
                "external": False,
                "func": True,
            }

        self._current_scope._local_symbols[sym] = sym_data

        node._scope = Scope(self._current_scope)
        self._current_scope = node._scope
        sym_data["scope"] = node._scope

        self.visit_sequence(node.decl)
        # Go over arguments and set them as dummy
        for arg in node.args:
            if not arg.arg in self._current_scope._local_symbols:
                raise SemanticError("Dummy variable '%s' not declared" % arg.arg)
            self._current_scope._local_symbols[arg.arg]["dummy"] = True
        # Check that non-dummy variables do not use the intent(..) attribute
        for sym in self._current_scope._local_symbols:
            s = self._current_scope._local_symbols[sym]
            if not s["dummy"]:
                if s["intent"]:
                    raise SemanticError("intent(..) is used for a non-dummy variable '%s'" % \
                        s["name"])

        # Iterate over nested functions
        self.visit_sequence(node.contains)

        self._current_scope = node._scope.parent_scope

    def visit_Program(self, node):
        node._scope = Scope(self._current_scope)
        self._current_scope = node._scope

        self.visit_sequence(node.decl)
        #self.visit_sequence(node.body)
        self.visit_sequence(node.contains)

        self._current_scope = node._scope.parent_scope

    def visit_Subroutine(self, node):
        node._scope = Scope(self._current_scope)
        self._current_scope = node._scope

        self.visit_sequence(node.args)
        self.visit_sequence(node.decl)
        #self.visit_sequence(node.body)
        self.visit_sequence(node.contains)

        self._current_scope = node._scope.parent_scope

def create_symbol_table(tree):
    v = SymbolTableVisitor()
    v.visit(tree)
    return v._global_scope



class ExprVisitor(ast.GenericASTVisitor):

    def __init__(self, global_scope):
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
        if not self._current_scope.resolve(node.id, False):
            raise UndeclaredVariableError("Variable '%s' not declared." \
                    % node.id)
        node._type = self._current_scope.resolve(node.id)["type"]

    def visit_FuncCallOrArray(self, node):
        if not self._current_scope.resolve(node.func, False):
            raise UndeclaredVariableError("Func or Array '%s' not declared." \
                    % node.func)
        self.visit_sequence(node.args)
        node._type = self._current_scope.resolve(node.func)["type"]

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
            if not self._current_scope.resolve(node.target.id, False):
                raise UndeclaredVariableError("Variable '%s' not declared." \
                        % node.target.id)
            node._type = self._current_scope.resolve(node.target.id)["type"]
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
                    raise TypeMismatch("Type mismatch: " \
                        "LHS=%s (%s), RHS=%s (%s)" % (node.target.id,
                        node._type, node.value, node.value._type))
        elif isinstance(node.target, ast.FuncCallOrArray):
            if not self._current_scope.resolve(node.target.func, False):
                raise UndeclaredVariableError("Array '%s' not declared." \
                        % node.target.func)
            node._type = self._current_scope.resolve(node.target.func)["type"]
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
    v = ExprVisitor(global_scope)
    v.visit(tree)
