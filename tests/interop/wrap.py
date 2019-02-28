# TODO: move this into the lfortran package itself
import sys
sys.path.append("../..")
from lfortran.ast import ast
from lfortran.ast.fortran_printer import print_fortran

from collections import namedtuple
import gzip

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
        if isinstance(b, Integer):
            if b.i == 1:
                return ""
            else:
                return str(b.i)
        elif isinstance(b, VarIdx):
            return self.symtab[b.idx].name
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
    assert isinstance(f, Procedure)
    fn = Function()
    assert isinstance(f.name, str)
    fn.name = f.name
    assert isinstance(f.type, str)
    fn.return_type = f.type
    fn.mangled_name = '__' + f.mod + '_MOD_' + f.name.lower()
    args = []
    for arg in f.args:
        assert isinstance(arg, VarIdx)
        args.append(convert_arg(table, arg.idx))
    fn.args = args
    return fn

def convert_module(table, public_symbols):
    m = Module()
    contains = []
    module_name = None
    for sym in public_symbols:
        s = table[sym.idx]
        if isinstance(s, Mod):
            # Skip modules if they are listed in public symbols
            continue
        assert isinstance(s, Procedure)
        if module_name:
            assert module_name == s.mod
        else:
            module_name = s.mod
        contains.append(convert_function(table, s))
    m.name = module_name
    m.contains = contains
    return m


class GFortranModuleParseError(Exception):
    pass

Mod = namedtuple("Module", ["name"])
PublicSymbol = namedtuple("PublicSymbol", ["name", "ambiguous_flag",
        "idx"])
Procedure = namedtuple("Procedure", ["name", "mod", "type", "args",
    "return_sym"])
Variable = namedtuple("Variable", ["name", "mod", "type", "bounds",
    "dummy", "intent"])
Integer = namedtuple("Integer", ["i"])
VarIdx = namedtuple("VarIdx", ["idx"])
Expr = namedtuple("Expr", [])

def str_(s):
    if not (isinstance(s, str) and len(s) >= 2 and s[0] == "'" and \
            s[-1] == "'"):
        raise GFortranModuleParseError("Invalid string")
    return s[1:-1]

def parse_type(t):
    if t[0] == "INTEGER":
        type_ = "integer"
    else:
        raise GFortranModuleParseError("Type not implemented")
    return type_

def parse_bound(b):
    if len(b) == 0:
        return None
    if b[0] == "CONSTANT":
        type_ = parse_type(b[1])
        val = str_(b[3])
        if type_ == "integer":
            return Integer(i=int(val))
        else:
            raise GFortranModuleParseError("Unsupported constant type")
    elif b[0] == "VARIABLE":
        var_index = b[3]
        return VarIdx(idx=var_index)
    elif b[0] == "OP":
        return Expr()
    raise GFortranModuleParseError("Unsupported bound type")

def parse_symbol(name, mod, info):
    sym_info, _, sym_type, _, _, dummy_args, sym_array, return_sym, \
            _, _, _, _ = info
    if sym_info[0] == "VARIABLE":
        if sym_array:
            ndim, _, atype = sym_array[:3]
            bounds = sym_array[3:]
            if len(bounds) != 2*ndim:
                raise GFortranModuleParseError("number of bounds not equal to "
                        "2*ndim")
            bounds2 = []
            for i in range(ndim):
                lb = parse_bound(bounds[2*i])
                ub = parse_bound(bounds[2*i+1])
                bounds2.append([lb, ub])
            assert len(bounds2) == ndim
            if atype == "ASSUMED_SHAPE":
                for b in bounds2:
                    if b[1] is not None:
                        raise GFortranModuleParseError("Assumed shape upper "
                            "bound must be None")
            elif atype == "EXPLICIT":
                for b in bounds2:
                    if b[1] is None:
                        raise GFortranModuleParseError("Explicit shape upper "
                            "bound must not be None")
        else:
            bounds2 = None
        if "DIMENSION" in sym_info:
            assert bounds2
        if sym_info[1] == "IN":
            intent = "in"
        elif sym_info[1] == "OUT":
            intent = "out"
        elif sym_info[1] == "UNKNOWN-INTENT":
            intent = None
        else:
            raise GFortranModuleParseError("Unknown intent")
        v = Variable(
                name=name,
                mod=mod,
                type=parse_type(sym_type),
                bounds=bounds2,
                dummy="DUMMY" in sym_info,
                intent=intent
            )
        return v
    elif sym_info[0] == "PROCEDURE":
        assert not sym_array
        if return_sym:
            return_sym = VarIdx(idx=return_sym)
        p = Procedure(
                name=name,
                mod=mod,
                type=parse_type(sym_type),
                args=[VarIdx(idx=x) for x in dummy_args],
                return_sym=return_sym
                )
        return p
    elif sym_info[0] == "MODULE":
        m = Mod(name=name)
        return m
    else:
        print(sym_info)
        raise GFortranModuleParseError("Unknown symbol")

#-------------------------------------------------------------------------------
#
# The following four functions were originally taken from:
# http://norvig.com/lispy.html
# https://github.com/norvig/pytudes/blob/6b430d707dfdbb80eba44813263cde504a68d419/py/lis.py
# They are licensed under an MIT license.
#
# Copyright (c) 2010-2017 Peter Norvig
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

def parse(program):
    "Read a Lisp expression from a string."
    return read_from_tokens(tokenize(program))

def tokenize(s):
    "Convert a string into a list of tokens."
    return s.replace('(',' ( ').replace(')',' ) ').split()

def read_from_tokens(tokens):
    "Read an expression from a sequence of tokens."
    if len(tokens) == 0:
        raise SyntaxError('unexpected EOF while reading')
    token = tokens.pop(0)
    if '(' == token:
        L = []
        while tokens[0] != ')':
            L.append(read_from_tokens(tokens))
        tokens.pop(0) # pop off ')'
        return L
    elif ')' == token:
        raise SyntaxError('unexpected )')
    else:
        return atom(token)

def atom(token):
    "Ints become ints; every other token is a str."
    try:
        return int(token)
    except ValueError:
        return str(token)

#-------------------------------------------------------------------------------


def load_module(filename):
    """
    Loads the gfortran module 'filename' and parses the data.

    The high level structure of the file as documented at [1] is parsed and the
    lisp syntax is transformed into lists of lists.

    Returns:

    version ..... gfortran module version
    orig_file ... the original Fortran file that produced the module
    symtab ...... the symbol table
    symtree ..... list of public symbols in the module

    The symbol table `symtab` is transformed into a dictionary, the key is the
    symbol number and the value is a particular named tuple, containing the
    details about the symbol.

    The list of public symbols `symtree` is a list of PublicSymbol named tuples
    containing the name, ambiguous flag and the symbol index (into `symtab`).

    Documentation for GFortran module files:
    [1] https://github.com/gcc-mirror/gcc/blob/898c6fe1170c03ca9f469ffa3565342942e96049/gcc/fortran/module.c#L22

    """
    with gzip.open(filename) as f:
        x = f.read().decode()
    lines = x.split("\n")
    header = lines[0]
    body = "(" + "\n".join(lines[1:]) + ")"
    h = header.split()
    if not h[0] == "GFORTRAN":
        raise GFortranModuleParseError("Not a GFortran module file")
    version = int(str_(h[3]))
    orig_file = h[-1]
    if version == 14:
        p = parse(body)
        if len(p) != 8:
            raise GFortranModuleParseError("Wrong number of items")
        _, _, _, _, _, _, symtab, symtree = p
        # Shape symtab into groups of 6
        if len(symtab) % 6 != 0:
            raise GFortranModuleParseError("symtab not multiple of 3")
        symtab = list(zip(*[iter(symtab)]*6))
        # Create a dictionary based on the index (first element in each group)
        symtab_dict = {}
        for s in symtab:
            symbol_number, symbol_name, module_name, _, _, symbol_info = s
            assert isinstance(symbol_number, int)
            assert symbol_number not in symtab_dict
            symtab_dict[symbol_number] = parse_symbol(str_(symbol_name),
                    str_(module_name), symbol_info)
        # Shape symtree into groups of 3
        if len(symtree) % 3 != 0:
            raise GFortranModuleParseError("symtree not multiple of 3")
        symtree = list(zip(*[iter(symtree)]*3))
        # Transform into a list of namedtuples
        symtree_list = []
        for s in symtree:
            symtree_list.append(PublicSymbol(str_(s[0]), s[1], s[2]))
        return version, orig_file, symtab_dict, symtree_list
    else:
        raise GFortranModuleParseError("Unsupported version")


version, orig_file, table, public_symbols = load_module("mod1.mod")
m = convert_module(table, public_symbols)
a = m.tofortran()
a.name = "mod2"
s = print_fortran(a)
print(s)
