"""
# Design

This file converts from a GFortran module file representation (documented in
the `gfort_mod_parser.py` module) to an LFortran abstract semantic
representation (ASR).

The ASR is designed to have the following features:

* ASR is still semantically equivalent to the original Fortran code (it did not
  lose any semantic information). ASR can be converted to AST, and AST to
  Fortran source code which is functionally equivalent to the original.

* ASR is as simple as possible: it does not contain any information that could
  not be inferred from ASR.

Note: Information that is lost when parsing source to AST: whitespace,
multiline/single line if statement distinction, case sensitivity of keywords.
Information that is lost when going from AST to ASR: detailed syntax how
variables were defined and the order of type attributes (whether array
dimension is using the `dimension` attribute, or parentheses at the variable;
or how many variables there are per declaration line or their order), as ASR
only represents the aggregated type information in the symbol table.

Note: ASR is the simplest way to generate Fortran code, as one does not
have to worry about the detailed syntax (as in AST) about how and where
things are declared. One specifies the symbol table for a module, then for
each symbol (functions, global variables, types, ...) one specifies the local
variables and if this is an interface then one needs to specify where one can
find an implementation, otherwise a body is supplied with statements, those
nodes are almost the same as in AST, except that each variable is just a
reference to a symbol in the symbol table (so by construction one cannot have
undefined variables). The symbol table for each node such as Function or Module
also references its parent (for example a function references a module,
a module references the global scope).

The ASR can be directly converted to an AST without gathering any other
information. And the AST directly to Fortran source code.

The ASR is always representing a semantically valid Fortran code. Part of
this is enforced by its construction, but part of it must be enforced with a
check (verification) after it is constructed.

When an ASR is used, it is assumed that it is valid (either explicitly
verified or constructed to be valid).

One can use ASR to gather further information about the program, for example
which procedures are pure, to introduce nodes for explicit type casting
(float to integer, and such) and one can optionally annotate/amend ASR with
such information.

The alternative solution is to check and gather all information as ASR is
constructed. For example when an ASR expression is constructed from variables
and numbers, it keeps track of the type and gives an exception when an
operation is not permitted by Fortran (such as adding numbers and characters)
and it automatically inserts explicit type casting nodes. When a function is
constructed, it automatically checks if it is pure. There still might
be some small check/verification that needs to be executed once ASR is
constructed.
"""

# TODO: move this into the lfortran package itself
import sys
sys.path.append("../..")
from lfortran.ast import ast
from lfortran.ast.fortran_printer import print_fortran
import gfort_mod_parser as gp

class Type(object):
    pass

class Intrinsic(Type):
    __slots__ = ["kind"]

    def __init__(self, kind=None):
        #super(Intrinsic, self).__init__()
        self.kind = kind

class Integer(Intrinsic):
    def __str__(self):
        return "integer"
class Real(Intrinsic):
    def __str__(self):
        return "real"
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
    __slots__ = ["type", "ndim", "atype", "bounds"]

class Arg:
    __slots__ = ["name", "intent", "type", "symtab", "symidx"]

    def get_decl(self):
        s = "%s, intent(%s) :: %s" % (self.type, self.intent, self.name)
        return s

    def tofortran_arg(self):
        return ast.arg(self.name)

    def tofortran_bound(self, b):
        if isinstance(b, gp.Integer):
            if b.i == 1:
                return ""
            else:
                return str(b.i)
        elif isinstance(b, gp.VarIdx):
            return self.symtab[b.idx].name
        print(b)
        raise NotImplementedError("Unsupported bound type")

    def tofortran_decl(self):
        attrs = [
            ast.Attribute(name="intent",
                    args=[ast.attribute_arg(arg=self.intent)]),
        ]
        if isinstance(self.type, Array):
            stype = str(self.type.type)
            args = []
            for i in range(self.type.ndim):
                if self.type.atype == "explicit_shape":
                    s = self.tofortran_bound(self.type.bounds[i][0])
                    if s != "":
                        s += ":"
                    s += self.tofortran_bound(self.type.bounds[i][1])
                    args.append(s)
                elif self.type.atype == "assumed_shape":
                    s = self.tofortran_bound(self.type.bounds[i][0])
                    s += ":"
                    args.append(s)
                else:
                    assert False
            args = [ast.attribute_arg(arg=x) for x in args]
            attrs.append(ast.Attribute(name="dimension", args=args))
        else:
            stype = str(self.type)
        decl = ast.decl(sym=self.name, sym_type=stype,
            dims=[], attrs=attrs)
        return ast.Declaration(vars=[decl], lineno=1, col_offset=1)

    def tofortran_cdecl(self):
        attrs = [
            ast.Attribute(name="intent",
                    args=[ast.attribute_arg(arg=self.intent)]),
        ]
        if isinstance(self.type, Array):
            args = []
            if self.type.atype == "assumed_shape":
                if self.type.ndim == 1:
                    stype = "type(c_desc1_t)"
                elif self.type.ndim == 2:
                    stype = "type(c_desc2_t)"
                else:
                    raise NotImplementedError("Assumed shape dim")
            else:
                stype = str(self.type.type)
                for i in range(self.type.ndim):
                    if self.type.atype == "explicit_shape":
                        s = self.tofortran_bound(self.type.bounds[i][0])
                        if s != "":
                            s += ":"
                        s += self.tofortran_bound(self.type.bounds[i][1])
                        args.append(s)
                    else:
                        assert False
                args = [ast.attribute_arg(arg=x) for x in args]
                attrs.append(ast.Attribute(name="dimension", args=args))
        else:
            stype = str(self.type)
        decl = ast.decl(sym=self.name, sym_type=stype,
            dims=[], attrs=attrs)
        return ast.Declaration(vars=[decl], lineno=1, col_offset=1)

class Function:
    __slots__ = ["name", "args", "return_type", "mangled_name"]

    def tofortran_impl(self):
        args = [x.tofortran_arg() for x in self.args]
        args_decl = [x.tofortran_decl() for x in self.args]
        iface_decl = [
            ast.Interface2(name="x", procs=[self.tofortran_iface()], lineno=1, col_offset=1)
        ]
        return_var = ast.Name(id="r", lineno=1, col_offset=1)
        cname = self.name + "_c_wrapper"
        cargs = []
        for x in self.args:
            a = ast.Name(id=x.name, lineno=1, col_offset=1)
            if isinstance(x.type, Array) and x.type.atype == "assumed_shape":
                cargs.append(
                    ast.FuncCallOrArray(
                        func="c_desc", args=[a], keywords=[],
                        lineno=1, col_offset=1
                    )
                )
            else:
                cargs.append(a)
        body = [
            ast.Assignment(
                target=return_var,
                value=ast.FuncCallOrArray(
                    func=cname, args=cargs, keywords=[],
                    lineno=1, col_offset=1
                ),
                lineno=1, col_offset=1
            )
        ]
        return ast.Function(name=self.name, args=args,
            return_type=str(self.return_type), return_var=return_var, bind=None,
            use=[],
            decl=args_decl+iface_decl, body=body, contains=[], lineno=1,
            col_offset=1)

    def tofortran_iface(self):
        args = [x.tofortran_arg() for x in self.args]
        args_decl = [x.tofortran_cdecl() for x in self.args]
        use = [
            ast.Use(module=ast.use_symbol(sym="gfort_interop", rename=None),
                symbols=[
                    ast.use_symbol(sym="c_desc1_t", rename=None),
                    ast.use_symbol(sym="c_desc2_t", rename=None),
                ], lineno=1, col_offset=1)
        ]
        cname = self.name + "_c_wrapper"
        bind_args = [ast.keyword(arg=None, value=ast.Name(id="c", lineno=1, col_offset=1)),
            ast.keyword(arg="name", value=ast.Str(s=self.mangled_name, lineno=1, col_offset=1))]
        bind = ast.Bind(args=bind_args)
        return ast.Function(name=cname, args=args,
            return_type=str(self.return_type), return_var=None,
            bind=bind, use=use,
            decl=args_decl, body=[], contains=[], lineno=1, col_offset=1)

class Module:
    __slots__ = ["name", "contains"]

    def tofortran(self):
        contains = [x.tofortran_impl() for x in self.contains]
        use = [
            ast.Use(module=ast.use_symbol(sym="gfort_interop", rename=None),
                symbols=[
                    ast.use_symbol(sym="c_desc", rename=None),
                ], lineno=1, col_offset=1)
        ]
        return ast.Module(name=self.name, use=use, decl=[], contains=contains)


def convert_arg(table, idx):
    arg = table[idx]
    a = Arg()
    assert isinstance(arg.name, str)
    a.name = arg.name
    assert isinstance(arg.intent, str)
    a.intent = arg.intent
    assert isinstance(table, dict)
    a.symtab = table
    assert isinstance(idx, int)
    a.symidx = idx
    assert isinstance(arg.type, str)
    type_ = arg.type
    if arg.bounds:
        ar = Array()
        ar.type = type_
        ar.ndim = len(arg.bounds)
        if arg.bounds[0][1] is None:
            ar.atype = "assumed_shape"
        else:
            ar.atype = "explicit_shape"
        ar.bounds = arg.bounds
        a.type = ar
    else:
        a.type = type_
    return a

def convert_function(table, f):
    assert isinstance(f, gp.Procedure)
    fn = Function()
    assert isinstance(f.name, str)
    fn.name = f.name
    assert isinstance(f.type, str)
    fn.return_type = f.type
    fn.mangled_name = '__' + f.mod + '_MOD_' + f.name.lower()
    args = []
    for arg in f.args:
        assert isinstance(arg, gp.VarIdx)
        args.append(convert_arg(table, arg.idx))
    fn.args = args
    return fn

def convert_module(table, public_symbols):
    m = Module()
    contains = []
    module_name = None
    for sym in public_symbols:
        s = table[sym.idx.idx]
        if isinstance(s, gp.Module):
            # Skip modules if they are listed in public symbols
            continue
        assert isinstance(s, gp.Procedure)
        if module_name:
            assert module_name == s.mod
        else:
            module_name = s.mod
        contains.append(convert_function(table, s))
    m.name = module_name
    m.contains = contains
    return m

version, orig_file, table, public_symbols = gp.load_module("mod1.mod")
m = convert_module(table, public_symbols)
a = m.tofortran()
a.name = "mod2"
s = print_fortran(a)
print(s)