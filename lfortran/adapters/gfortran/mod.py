"""
# Module for parsing GFortran module files.

## Usage:

Load the module file using:

>>> version, orig_filename, symtab, public_symbols = load_module("filename.mod")

The `symtab` will contain the symbol table, `public_symbols` contains the list
of module symbols that are public, `orig_filename` is the name of the original
Fortran module source code (e.g., "filename.f90") and `version` is the GFortran
module version (e.g., 14).

## Design Motivation

This module is self-contained, it does not depend on the rest of LFortran. It
parses the GFortran module file into a representation with the following
properties:

* It contains all the information from the original GFortran module file
  (nothing got lost), so that in principle the original file can be
  reconstructed (the only thing lost would be white space and the order of the
  symbols).

* It is as simple as possible: it does not contain any additional
  information that could be inferred.

* Implemented using natural Python datastructures/syntax: it uses a Python
  dicitonary, lists and namedtuples to store the data.

In this sense it an abstract representation of the module file that is suitable
as the basis for any tools that need to work with the GFortran module file.

## Details

The symbol table `symtab` is a dictionary, with keys being the symbol index
(number). The following symbol types are permitted: Procedure, Variable, Module.
Each is represented by a namedtuple. Procedure and Variable have some common
fields:

* name: name of the symbol
* mod: which module it belongs to
* type: type of the symbol

Procedure has `args` and `return_sym` fields, which contain VarIdx namedtuples.
Each variable is uniquely indentified by its symbol index, which can be used to
look it up in the `symtab` dictionary. The VarIdx namedtuple represents this
symbol index.

If the variable is an array, then the `bounds` field is not None and it
contains the lower and upper bound for each dimension (as lists of lists). If
the upper bound is None, it is an assumed-shape array, otherwise it is an
explicit-shape array. The array bound can be an integer (represented by the
Integer namedtuple), or it can be a variable (represented by VarIdx) or an
expression (represented by Expr).

The `public_symbols` variable contains the list of public module symbols,
represented by the PublicSymbol namedtuple.


Update: Use the `mod_to_asr()` function that returns LFortran's ASR directly.
That way all the semantics is already in the (verified) ASR, and one can use
all the tooling around ASR right away without needing to learn about some
internal representation in this module that is specific to GFortran.

## GFortran ABI

GFortran ABI is stable for a given GFortran module file version. The module
file version stays the same between releases (4.5.0 and 4.5.2), but usually
changes between minor versions (4.5 vs. 4.6). Here is a correspondence of
GFortran compiler versions and GFortran module file versions:

GFortran version    module file version
---------------------------------------
<= 4.3              unversioned
4.4 (Apr 2009)       0
4.5 (Apr 2010)       4
4.6 (Mar 2011)       6
4.7 (Mar 2012)       9
4.8 (Mar 2013)      10
4.9 (Apr 2014)      12
5.x (Apr 2015)      14
6.x (Apr 2016)      14
7.x (May 2017)      14
8.x (May 2018)      15
9.x (??? 2019)      ??

The GFortran array descriptor is defined in libgfortran.h.
Module file version 15:

https://github.com/gcc-mirror/gcc/blob/gcc-8_1_0-release/libgfortran/libgfortran.h#L324

    typedef struct descriptor_dimension
    {
        index_type _stride;
        index_type lower_bound;
        index_type _ubound;
    }
    descriptor_dimension;

    typedef struct dtype_type
    {
        size_t elem_len;
        int version;
        signed char rank;
        signed char type;
        signed short attribute;
    }
    dtype_type;

    #define GFC_FULL_ARRAY_DESCRIPTOR(r, type) \
    struct {\
        type *base_addr;\
        size_t offset;\
        dtype_type dtype;\
        index_type span;\
        descriptor_dimension dim[r];\
    }

Module file versions 0..14 (changes from 15: no `span` member, `dtype` is
an integer instead of a struct):

https://github.com/gcc-mirror/gcc/blob/gcc-7_1_0-release/libgfortran/libgfortran.h#L328
https://github.com/gcc-mirror/gcc/blob/gcc-4_4_0-release/libgfortran/libgfortran.h#L298

    typedef struct descriptor_dimension
    {
        index_type _stride;
        index_type lower_bound;
        index_type _ubound;
    }
    descriptor_dimension;

    #define GFC_ARRAY_DESCRIPTOR(r, type) \
    struct {\
        type *base_addr;\
        size_t offset;\
        index_type dtype;\
        descriptor_dimension dim[r];\
    }

Then there is the CFI array descriptor from ISO_Fortran_binding.h:

https://github.com/gcc-mirror/gcc/blob/c3ce5d657bac95a3ac5c0457ac48b47a9710c838/libgfortran/ISO_Fortran_binding.h#L69

    typedef struct CFI_dim_t
    {
        CFI_index_t lower_bound;
        CFI_index_t extent;
        CFI_index_t sm;
    }
    CFI_dim_t;

    #define CFI_CDESC_TYPE_T(r, base_type) \
        struct { \
            base_type *base_addr; \
            size_t elem_len; \
            int version; \
            CFI_rank_t rank; \
            CFI_attribute_t attribute; \
            CFI_type_t type; \
            CFI_dim_t dim[r]; \
    }

GFortran then converts between its own array descriptor and the CFI
descriptor, for example here is the code that converts from CFI to
GFortran:

https://github.com/gcc-mirror/gcc/blob/c3ce5d657bac95a3ac5c0457ac48b47a9710c838/libgfortran/runtime/ISO_Fortran_binding.c#L37

"""

from collections import namedtuple
import gzip

from lfortran.asr import asr
from lfortran.asr.asr_check import verify_asr
from lfortran.asr.builder import (make_translation_unit,
    translation_unit_make_module, scope_add_function, make_type_integer,
    make_type_real, type_eq, make_binop, scope_add_symbol)


def string_to_type(stype, kind=None):
    """
    Converts a string `stype` and a numerical `kind` to an ASR type.
    """
    if stype == "integer":
        return make_type_integer(kind)
    elif stype == "real":
        return make_type_real(kind)
    elif stype == "complex":
        raise NotImplementedError("Complex not implemented")
    elif stype == "character":
        raise NotImplementedError("Complex not implemented")
    elif stype == "logical":
        raise NotImplementedError("Complex not implemented")
    raise Exception("Unknown type")

def tofortran_bound(symtab, lscope, b):
    if isinstance(b, Integer):
        return asr.Num(n=b.i, type=make_type_integer())
    elif isinstance(b, VarIdx):
        v = symtab[b.idx]
        w = lscope.resolve(v.name)
        assert type_eq(w.type, string_to_type(v.type))
        return w
    raise NotImplementedError("Unsupported bound type")

def convert_arg(table, lscope, idx):
    arg = table[idx]
    assert isinstance(arg.name, str)
    assert isinstance(arg.type, str)
    a = asr.Variable(name=arg.name, type=string_to_type(arg.type), dummy=True)
    assert isinstance(arg.intent, str)
    a.intent = arg.intent

    if arg.bounds:
        dims = []
        for bound in arg.bounds:
            lb, ub = bound
            if lb:
                lb = tofortran_bound(table, lscope, lb)
            if ub:
                ub = tofortran_bound(table, lscope, ub)
            dims.append(asr.dimension(lb, ub))
        a.type.dims = dims
    return a

def convert_function(symtab, table, f):
    assert isinstance(f, Procedure)
    assert isinstance(f.name, str)
    assert isinstance(f.type, str)
    return_var = asr.Variable(name=f.name, type=string_to_type(f.type))
    lf = scope_add_function(symtab, f.name, return_var=return_var)
    args = []
    for arg in f.args:
        assert isinstance(arg, VarIdx)
        larg = convert_arg(table, lf.symtab, arg.idx)
        scope_add_symbol(lf.symtab, larg)
        args.append(larg)
    lf.args = args

def convert_module(table, public_symbols):
    # Determine the module name
    module_name = None
    for sym in public_symbols:
        s = table[sym.idx.idx]
        if isinstance(s, Module):
            # Skip modules if they are listed in public symbols
            continue
        assert isinstance(s, Procedure)
        if module_name:
            assert module_name == s.mod
        else:
            module_name = s.mod


    u = make_translation_unit()
    m = translation_unit_make_module(u, module_name)

    # Convert functions
    for sym in public_symbols:
        s = table[sym.idx.idx]
        if isinstance(s, Module):
            continue
        assert isinstance(s, Procedure)
        convert_function(m.symtab, table, s)

    return u




class GFortranModuleParseError(Exception):
    pass

Module = namedtuple("Module", ["name"])
Procedure = namedtuple("Procedure", ["name", "mod", "type", "args",
    "return_sym"])
Variable = namedtuple("Variable", ["name", "mod", "type", "bounds",
    "dummy", "intent"])

VarIdx = namedtuple("VarIdx", ["idx"])

Integer = namedtuple("Integer", ["i"])
Expr = namedtuple("Expr", [])

PublicSymbol = namedtuple("PublicSymbol", ["name", "ambiguous_flag",
        "idx"])

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
        m = Module(name=name)
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
            symtree_list.append(PublicSymbol(str_(s[0]), s[1],
                VarIdx(idx=s[2])))
        return version, orig_file, symtab_dict, symtree_list
    else:
        raise GFortranModuleParseError("Unsupported version")

def mod_to_asr(filename):
    version, orig_file, table, public_symbols = load_module(filename)
    u = convert_module(table, public_symbols)
    verify_asr(u)
    return u
