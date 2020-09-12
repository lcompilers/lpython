"""
Generate C++ AST node definitions from an ASDL description.
"""

import sys
import os
import asdl


class ASDLVisitor(asdl.VisitorBase):

    def __init__(self, stream, data):
        super(ASDLVisitor, self).__init__()
        self.stream = stream
        self.data = data

    def visitModule(self, mod, *args):
        for df in mod.dfns:
            self.visit(df, *args)

    def visitSum(self, sum, *args):
        for tp in sum.types:
            self.visit(tp, *args)

    def visitType(self, tp, *args):
        self.visit(tp.value, *args)

    def visitProduct(self, prod, *args):
        for field in prod.fields:
            self.visit(field, *args)

    def visitConstructor(self, cons, *args):
        for field in cons.fields:
            self.visit(field, *args)

    def visitField(self, field, *args):
        pass

    def emit(self, line, level=0):
        indent = "    "*level
        self.stream.write(indent + line + "\n")


def is_simple_sum(sum):
    """
    Returns true if `sum` is a simple sum.

    Example of a simple sum:

        boolop = And | Or

    Example of not a simple sum:

        type
            = Integer(int kind)
            | Real(int kind)

    """
    assert isinstance(sum, asdl.Sum)
    for constructor in sum.types:
        if constructor.fields:
            return False
    return True

def attr_to_args(attrs):
    args = []
    for attr in attrs:
        kw = ""
        if attr.type == "int":
            if attr.name in ["lineno", "col_offset"]:
                kw = "=1"
            else:
                kw = "=0"
        elif attr.type in ["string", "identifier"]:
            kw = '=None'
        elif attr.seq:
            kw = "=[]"
        else:
            kw = "=None"
        args.append(attr.name + kw)
    return ", ".join(args)

simple_sums = []
sums = []
products = []
subs = {}

def convert_type(asdl_type, seq, opt, mod_name):
    if asdl_type in simple_sums:
        type_ = asdl_type + "Type"
        assert not seq
    elif asdl_type == "string":
        type_ = "char*"
        assert not seq
    elif asdl_type == "identifier":
        type_ = "char*"
        if seq:
            # List of strings is **
            type_ = type_ + "*"
    elif asdl_type == "constant":
        type_ = "bool"
        assert not seq
    elif asdl_type == "object":
        # FIXME: this should use some other type that we actually need
        type_ = "int /* object */"
        if seq:
            type_ = type_ + "*"
    elif asdl_type == "node":
        type_ = "%s_t*" % mod_name
        if seq:
            type_ = type_ + "*"
    elif asdl_type == "symbol_table":
        type_ = "SymbolTable*"
    elif asdl_type == "int":
        type_ = "int"
        assert not seq
    else:
        type_ = asdl_type + "_t"
        if asdl_type in products:
            # Product type
            # Not a pointer by default
            if seq or opt:
                # Sequence or an optional argument must be a pointer
                type_ = type_ + "*"
        else:
            # Sum type
            # Sum type is polymorphic, must be a pointer
            type_ = type_ + "*"
            if seq:
                # Sequence of polymorphic types must be a double pointer
                type_ = type_ + "*"
    return type_

class CollectVisitor(ASDLVisitor):

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitSum(self, sum, base):
        if not is_simple_sum(sum):
            sums.append(base);

class ASTNodeVisitor0(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Forward declarations")
        self.emit("")
        super(ASTNodeVisitor0, self).visitModule(mod)

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitSum(self, sum, base):
        if is_simple_sum(sum):
            simple_sums.append(base)
            self.emit("enum %sType // Simple Sum" % base)
            self.emit("{ // Types");
            s = [cons.name for cons in sum.types]
            self.emit(    ", ".join(s), 1)
            self.emit("};");
        else:
            self.emit("struct %s_t; // Sum" % base)

    def visitProduct(self, product, name):
        products.append(name)
        self.emit("struct %s_t; // Product" % name)


class ASTNodeVisitor1(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Products declarations")
        self.emit("")
        self.mod = mod
        super(ASTNodeVisitor1, self).visitModule(mod)

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitProduct(self, product, name):
        self.emit("struct %s_t // Product" % name)
        self.emit("{");
        for f in product.fields:
            type_ = convert_type(f.type, f.seq, f.opt, self.mod.name.lower())
            if f.seq:
                seq = " size_t n_%s; // Sequence" % f.name
            else:
                seq = ""
            self.emit("%s m_%s;%s" % (type_, f.name, seq), 1)
        self.emit("};");


class ASTNodeVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Sums declarations")
        self.emit("")
        self.mod = mod
        super(ASTNodeVisitor, self).visitModule(mod)

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitSum(self, sum, base):
        if not is_simple_sum(sum):
            self.emit("enum %sType // Types" % base)
            self.emit("{");
            s = [cons.name for cons in sum.types]
            self.emit(    ", ".join(s), 1)
            self.emit("};");
            self.emit("")
            self.emit("struct %s_t // Sum" % base)
            self.emit("{")
            mod = subs["mod"]
            self.emit(    "const static %sType class_type = %sType::%s;" \
                    % (mod, mod, base), 1)
            self.emit(    "%(mod)s_t base;" % subs, 1)
            self.emit(    "%sType type;" % base, 1)
            self.emit("};")
            self.emit("")
            for cons in sum.types:
                self.visit(cons, base, sum.attributes)
            self.emit("")
            self.emit("")

    def visitConstructor(self, cons, base, extra_attributes):
        self.emit("struct %s_t // Constructor" % cons.name, 1)
        self.emit("{", 1);
        self.emit(    "const static %sType class_type = %sType::%s;" \
                % (base, base, cons.name), 2)
        self.emit(    "typedef %s_t parent_type;" % base, 2)
        self.emit(    "%s_t base;" % base, 2);
        args = ["Allocator &al", "const Location &a_loc"]
        lines = []
        for f in cons.fields:
            type_ = convert_type(f.type, f.seq, f.opt, self.mod.name.lower())
            if f.seq:
                seq = " size_t n_%s; // Sequence" % f.name
            else:
                seq = ""
            self.emit("%s m_%s;%s" % (type_, f.name, seq), 2)
            args.append("%s a_%s" % (type_, f.name))
            lines.append("n->m_%s = a_%s;" % (f.name, f.name))
            if f.seq:
                args.append("size_t n_%s" % (f.name))
                lines.append("n->n_%s = n_%s;" % (f.name, f.name))
        self.emit("};", 1)
        self.emit("static inline %s_t* make_%s_t(%s) {" % (subs["mod"],
            cons.name, ", ".join(args)), 1)
        self.emit(    "%s_t *n;" % cons.name, 2)
        self.emit(    "n = al.make_new<%s_t>();" % cons.name, 2)
        self.emit(    "n->base.type = %sType::%s;" % (base, cons.name), 2)
        self.emit(    "n->base.base.type = %sType::%s;" % (subs["mod"],
            base), 2)
        self.emit(    "n->base.base.loc = a_loc;", 2)
        for line in lines:
            self.emit(line, 2)
        self.emit(    "return (%(mod)s_t*)n;" % subs, 2)
        self.emit("}", 1)
        self.emit("")

class ASTVisitorVisitor1(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Visitor functions")
        self.emit("")
        super(ASTVisitorVisitor1, self).visitModule(mod)

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitSum(self, sum, base):
        if not is_simple_sum(sum):
            self.emit("template <class Visitor>")
            self.emit("static void visit_%s_t(const %s_t &x, Visitor &v) {" \
                    % (base, base))
            self.emit(    "LFORTRAN_ASSERT(x.base.type == %sType::%s)" \
                    % (subs["mod"], base), 1)
            self.emit(    "switch (x.type) {", 1)
            for type_ in sum.types:
                self.emit("        case %sType::%s: { v.visit_%s((const %s_t &)x);"
                    " return; }" % (base, type_.name, type_.name, type_.name))
            self.emit("    }")
            self.emit("}")
            self.emit("")

class ASTVisitorVisitor1b(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("template <class Visitor>")
        self.emit("static void visit_%(mod)s_t(const %(mod)s_t &x, Visitor &v) {" % subs)
        self.emit("    switch (x.type) {")
        for type_ in sums:
            self.emit("        case %sType::%s: { v.visit_%s((const %s_t &)x);"
                " return; }" % (subs["mod"], type_, type_, type_))
        self.emit("    }")
        self.emit("}")
        self.emit("")

class ASTVisitorVisitor2(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Visitor base class")
        self.emit("")
        self.emit("template <class Derived>")
        self.emit("class BaseVisitor")
        self.emit("{")
        self.emit("private:")
        self.emit("    Derived& self() { return static_cast<Derived&>(*this); }")
        self.emit("public:")
        self.emit(    "void visit_%(mod)s(const %(mod)s_t &b) { visit_%(mod)s_t(b, self()); }" % subs, 1)
        super(ASTVisitorVisitor2, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitSum(self, sum, base):
        if not is_simple_sum(sum):
            self.emit("void visit_%s(const %s_t &b) { visit_%s_t(b, self()); }"\
                    % (base, base, base), 1)
            for type_ in sum.types:
                self.emit("""void visit_%s(const %s_t & /* x */) { throw LFortran::LFortranException("visit_%s() not implemented"); }""" \
                        % (type_.name, type_.name, type_.name), 2)


class ASTWalkVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Walk Visitor base class")
        self.emit("")
        self.emit("template <class Derived>")
        self.emit("class BaseWalkVisitor : public BaseVisitor<Derived>")
        self.emit("{")
        self.emit("private:")
        self.emit("    Derived& self() { return static_cast<Derived&>(*this); }")
        self.emit("public:")
        super(ASTWalkVisitorVisitor, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(ASTWalkVisitorVisitor, self).visitType(tp, tp.name)

    def visitProduct(self, prod, name):
        self.make_visitor(name, prod.fields)

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        self.emit("void visit_%s(const %s_t &x) {" % (name, name), 1)
        self.used = False
        have_body = False
        for field in fields:
            self.visitField(field)
        if not self.used:
            # Note: a better solution would be to change `&x` to `& /* x */`
            # above, but we would need to change emit to return a string.
            self.emit("if ((bool&)x) { } // Suppress unused warning", 2)
        self.emit("}", 1)

    def visitField(self, field):
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            level = 2
            if field.type in products:
                if field.opt:
                    template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
                else:
                    template = "self().visit_%s(x.m_%s);" % (field.type, field.name)
            else:
                template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
            self.used = True
            if field.seq:
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                if field.type in products:
                    self.emit("    self().visit_%s(x.m_%s[i]);" % (field.type, field.name), level)
                else:
                    self.emit("    self().visit_%s(*x.m_%s[i]);" % (field.type, field.name), level)
                self.emit("}", level)
                return
            elif field.opt:
                self.emit("if (x.m_%s)" % field.name, 2)
                level = 3
            self.emit(template, level)


class PickleVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Walk Visitor base class")
        self.emit("")
        self.emit("template <class Derived>")
        self.emit("class PickleBaseVisitor : public BaseVisitor<Derived>")
        self.emit("{")
        self.emit("public:")
        self.emit(  "std::string s;", 1)
        self.emit(  "bool use_colors;", 1)
        self.emit("public:")
        self.emit(  "PickleBaseVisitor() : use_colors(false) { s.reserve(100000); }", 1)
        self.mod = mod
        super(PickleVisitorVisitor, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        super(PickleVisitorVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        assert isinstance(sum, asdl.Sum)
        if is_simple_sum(sum):
            name = args[0] + "Type"
            self.make_simple_sum_visitor(name, sum.types)
        else:
            for tp in sum.types:
                self.visit(tp, *args)

    def visitProduct(self, prod, name):
        self.make_visitor(name, prod.fields, False)

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields, True)

    def make_visitor(self, name, fields, cons):
        self.emit("void visit_%s(const %s_t &x) {" % (name, name), 1)
        self.emit(    's.append("(");', 2)
        subs = {
            "Assignment": "=",
            "Associate": "=>",
        }
        if name in subs:
            name = subs[name]
        if cons:
            self.emit(    'if (use_colors) {', 2)
            self.emit(        's.append(color(style::bold));', 3)
            self.emit(        's.append(color(fg::magenta));', 3)
            self.emit(    '}', 2)
            self.emit(    's.append("%s");' % name, 2)
            self.emit(    'if (use_colors) {', 2)
            self.emit(        's.append(color(fg::reset));', 3)
            self.emit(        's.append(color(style::reset));', 3)
            self.emit(    '}', 2)
            if len(fields) > 0:
                self.emit(    's.append(" ");', 2)
        self.used = False
        for n, field in enumerate(fields):
            self.visitField(field, cons)
            if n < len(fields) - 1:
                self.emit(    's.append(" ");', 2)
        self.emit(    's.append(")");', 2)
        if not self.used:
            # Note: a better solution would be to change `&x` to `& /* x */`
            # above, but we would need to change emit to return a string.
            self.emit("if ((bool&)x) { } // Suppress unused warning", 2)
        self.emit("}", 1)

    def make_simple_sum_visitor(self, name, types):
        self.emit("void visit_%s(const %s &x) {" % (name, name), 1)
        self.emit(    'if (use_colors) {', 2)
        self.emit(        's.append(color(style::bold));', 3)
        self.emit(        's.append(color(fg::green));', 3)
        self.emit(    '}', 2)
        self.emit(    'switch (x) {', 2)
        for tp in types:
            self.emit(    'case (%s::%s) : { s.append("%s"); break; }' \
                % (name, tp.name, tp.name), 3)
        self.emit(    '}', 2)
        self.emit(    'if (use_colors) {', 2)
        self.emit(        's.append(color(fg::reset));', 3)
        self.emit(        's.append(color(style::reset));', 3)
        self.emit(    '}', 2)
        self.emit("}", 1)

    def visitField(self, field, cons):
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            self.used = True
            level = 2
            if field.type in products:
                if field.opt:
                    template = "this->visit_%s(*x.m_%s);" % (field.type, field.name)
                else:
                    template = "this->visit_%s(x.m_%s);" % (field.type, field.name)
            else:
                template = "this->visit_%s(*x.m_%s);" % (field.type, field.name)
            if field.seq:
                self.emit('s.append("[");', level)
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                if field.type in sums:
                    self.emit("this->visit_%s(*x.m_%s[i]);" % (field.type, field.name), level+1)
                else:
                    self.emit("this->visit_%s(x.m_%s[i]);" % (field.type, field.name), level+1)
                self.emit(    'if (i < x.n_%s-1) s.append(" ");' % (field.name), level+1)
                self.emit("}", level)
                self.emit('s.append("]");', level)
            elif field.opt:
                self.emit("if (x.m_%s) {" % field.name, 2)
                self.emit(template, 3)
                self.emit("} else {", 2)
                self.emit(    's.append("()");', 3)
                self.emit("}", 2)
            else:
                self.emit(template, level)
        else:
            if field.type == "identifier":
                if field.seq:
                    assert not field.opt
                    level = 2
                    self.emit('s.append("[");', level)
                    self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                    self.emit("s.append(x.m_%s[i]);" % (field.name), level+1)
                    self.emit(    'if (i < x.n_%s-1) s.append(" ");' % (field.name), level+1)
                    self.emit("}", level)
                    self.emit('s.append("]");', level)
                else:
                    if field.opt:
                        self.emit("if (x.m_%s) {" % field.name, 2)
                        self.emit(    's.append(x.m_%s);' % field.name, 3)
                        self.emit("} else {", 2)
                        self.emit(    's.append("()");', 3)
                        self.emit("}", 2)
                    else:
                        self.emit('s.append(x.m_%s);' % field.name, 2)
            elif field.type == "node":
                assert not field.opt
                assert field.seq
                level = 2
                self.emit('s.append("[");', level)
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                mod_name = self.mod.name.lower()
                self.emit("this->visit_%s(*x.m_%s[i]);" % (mod_name, field.name), level+1)
                self.emit(    'if (i < x.n_%s-1) s.append(" ");' % (field.name), level+1)
                self.emit("}", level)
                self.emit('s.append("]");', level)
            elif field.type == "symbol_table":
                assert not field.opt
                assert not field.seq
                if field.name == "parent_symtab":
                    level = 2
                    self.emit('s.append(x.m_%s->get_hash());' % field.name, level)
                else:
                    level = 2
                    self.emit('s.append("(");', level)
                    self.emit('if (use_colors) {', level)
                    self.emit(    's.append(color(fg::yellow));', level+1)
                    self.emit('}', level)
                    self.emit('s.append("SymbolTable");', level)
                    self.emit('if (use_colors) {', level)
                    self.emit(    's.append(color(fg::reset));', level+1)
                    self.emit('}', level)
                    self.emit('s.append(" ");', level)
                    self.emit('s.append(x.m_%s->get_hash());' % field.name, level)
                    self.emit('s.append(" {");', level)
                    self.emit('{', level)
                    self.emit('    size_t i = 0;', level)
                    self.emit('    for (auto &a : x.m_%s->scope) {' % field.name, level)
                    self.emit('        s.append(a.first + ": ");', level)
                    self.emit('        this->visit_asr(*a.second);', level)
                    self.emit('        if (i < x.m_%s->scope.size()-1) s.append(", ");' % field.name, level)
                    self.emit('        i++;', level)
                    self.emit('    }', level)
                    self.emit('}', level)
                    self.emit('s.append("})");', level)
            elif field.type == "string" and not field.seq:
                if field.opt:
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    's.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    's.append("()");', 3)
                    self.emit("}", 2)
                else:
                    self.emit('s.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 2)
            elif field.type == "int" and not field.seq:
                if field.opt:
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    's.append(std::to_string(x.m_%s));' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    's.append("()");', 3)
                    self.emit("}", 2)
                else:
                    self.emit('s.append(std::to_string(x.m_%s));' % field.name, 2)
            elif field.type in self.data.simple_types:
                if field.opt:
                    self.emit('s.append("Unimplementedopt");', 2)
                else:
                    self.emit('visit_%sType(x.m_%s);' \
                            % (field.type, field.name), 2)
            else:
                self.emit('s.append("Unimplemented' + field.type + '");', 2)



class ASDLData(object):

    def __init__(self, tree):
        simple_types = set()
        prod_simple = set()
        field_masks = {}
        required_masks = {}
        optional_masks = {}
        cons_attributes = {}
        def add_masks(fields, node):
            required_mask = 0
            optional_mask = 0
            for i, field in enumerate(fields):
                flag = 1 << i
                if field not in field_masks:
                    field_masks[field] = flag
                else:
                    assert field_masks[field] == flag
                if field.opt:
                    optional_mask |= flag
                else:
                    required_mask |= flag
            required_masks[node] = required_mask
            optional_masks[node] = optional_mask
        for tp in tree.dfns:
            if isinstance(tp.value, asdl.Sum):
                sum = tp.value
                if is_simple_sum(sum):
                    simple_types.add(tp.name)
                else:
                    attrs = [field for field in sum.attributes]
                    for cons in sum.types:
                        add_masks(attrs + cons.fields, cons)
                        cons_attributes[cons] = attrs
            else:
                prod = tp.value
                prod_simple.add(tp.name)
                add_masks(prod.fields, prod)
        prod_simple.update(simple_types)
        self.cons_attributes = cons_attributes
        self.simple_types = simple_types
        self.prod_simple = prod_simple
        self.field_masks = field_masks
        self.required_masks = required_masks
        self.optional_masks = optional_masks


HEAD = r"""#ifndef LFORTRAN_%(MOD)s_H
#define LFORTRAN_%(MOD)s_H

// Generated by grammar/asdl_cpp.py

#include <lfortran/parser/alloc.h>
#include <lfortran/parser/location.h>
#include <lfortran/colors.h>
#include <lfortran/exception.h>
#include <lfortran/semantics/asr_scopes.h>


namespace LFortran::%(MOD)s {

enum %(mod)sType
{
    %(types)s
};

struct %(mod)s_t
{
    %(mod)sType type;
    Location loc;
};


template <class T, class U>
inline bool is_a(const U &x)
{
    return T::class_type == x.type;
}

// Cast one level down

template <class T, class U>
static inline T* down_cast(const U *f)
{
    LFORTRAN_ASSERT(is_a<T>(*f));
    return (T*)f;
}

// Cast two levels down

template <class T>
static inline T* down_cast2(const %(mod)s_t *f)
{
    typedef typename T::parent_type ptype;
    LFORTRAN_ASSERT(is_a<ptype>(*f));
    ptype *t = (ptype *)f;
    LFORTRAN_ASSERT(is_a<T>(*t));
    return (T*)t;
}


"""

FOOT = r"""} // namespace LFortran::%(MOD)s

#endif // LFORTRAN_%(MOD)s_H
"""

visitors = [ASTNodeVisitor0, ASTNodeVisitor1, ASTNodeVisitor,
        ASTVisitorVisitor1, ASTVisitorVisitor1b, ASTVisitorVisitor2,
        ASTWalkVisitorVisitor, PickleVisitorVisitor]


def main(argv):
    if len(argv) == 3:
        def_file, out_file = argv[1:]
    elif len(argv) == 1:
        print("Assuming default values of AST.asdl and ast.h")
        here = os.path.dirname(__file__)
        def_file = os.path.join(here, "AST.asdl")
        out_file = os.path.join(here, "..", "src", "lfortran", "ast.h")
    else:
        print("invalid arguments")
        return 2
    mod = asdl.parse(def_file)
    data = ASDLData(mod)
    CollectVisitor(None, data).visit(mod)
    types_ = ", ".join(sums)
    global subs
    subs = {
        "MOD": mod.name.upper(),
        "mod": mod.name.lower(),
        "types": types_,
    }
    fp = open(out_file, "w")
    try:
        fp.write(HEAD % subs)
        for visitor in visitors:
            visitor(fp, data).visit(mod)
            fp.write("\n\n")
        fp.write(FOOT % subs)
    finally:
        fp.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv))
