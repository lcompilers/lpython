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
    elif asdl_type == "bool":
        type_ = "bool"
        assert not seq
    elif asdl_type == "float":
        type_ = "double"
        assert not seq
    elif asdl_type == "node":
        type_ = "%s_t*" % mod_name
        if seq:
            type_ = type_ + "*"
    elif asdl_type == "symbol_table":
        type_ = "SymbolTable*"
    elif asdl_type == "int":
        type_ = "int64_t"
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
        self.emit(    "Location loc;", 1);
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
            if f.name in ["global_scope", "symtab"]:
                lines.append("a_%s->asr_owner = (asr_t*)n;" % (f.name))
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
            self.emit(    "LCOMPILERS_ASSERT(x.base.type == %sType::%s)" \
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
        self.emit("template <class Struct>")
        self.emit("class BaseVisitor")
        self.emit("{")
        self.emit("private:")
        self.emit("    Struct& self() { return static_cast<Struct&>(*this); }")
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
                self.emit("""void visit_%s(const %s_t & /* x */) { throw LCompilersException("visit_%s() not implemented"); }""" \
                        % (type_.name, type_.name, type_.name), 2)


class ASTWalkVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Walk Visitor base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class BaseWalkVisitor : public BaseVisitor<Struct>")
        self.emit("{")
        self.emit("private:")
        self.emit("    Struct& self() { return static_cast<Struct&>(*this); }")
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
            if field.seq:
                self.used = True
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                if field.type in products:
                    self.emit("    self().visit_%s(x.m_%s[i]);" % (field.type, field.name), level)
                else:
                    if field.type != "symbol":
                        self.emit("    self().visit_%s(*x.m_%s[i]);" % (field.type, field.name), level)
                self.emit("}", level)
            else:
                if field.type in products:
                    self.used = True
                    if field.opt:
                        self.emit("if (x.m_%s)" % field.name, 2)
                        level = 3
                    if field.opt:
                        self.emit("self().visit_%s(*x.m_%s);" % (field.type, field.name), level)
                    else:
                        self.emit("self().visit_%s(x.m_%s);" % (field.type, field.name), level)
                else:
                    if field.type != "symbol":
                        self.used = True
                        if field.opt:
                            self.emit("if (x.m_%s)" % field.name, 2)
                            level = 3
                        self.emit("self().visit_%s(*x.m_%s);" % (field.type, field.name), level)
        elif field.type == "symbol_table" and field.name in["symtab",
                "global_scope"]:
            self.used = True
            self.emit("for (auto &a : x.m_%s->get_scope()) {" % field.name, 2)
            self.emit(  "this->visit_symbol(*a.second);", 3)
            self.emit("}", 2)

class CallReplacerOnExpressionsVisitor(ASDLVisitor):

    def __init__(self, stream, data):
        self.current_expr_copy_variable_count = 0
        super(CallReplacerOnExpressionsVisitor, self).__init__(stream, data)

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Walk Visitor base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class CallReplacerOnExpressionsVisitor : public BaseVisitor<Struct>")
        self.emit("{")
        self.emit("private:")
        self.emit("    Struct& self() { return static_cast<Struct&>(*this); }")
        self.emit("public:")
        self.emit("    ASR::expr_t** current_expr;")
        self.emit("    SymbolTable* current_scope;")
        self.emit("")
        self.emit("    void call_replacer() {}")
        self.emit("    void transform_stmts(ASR::stmt_t **&m_body, size_t &n_body) {")
        self.emit("    for (size_t i = 0; i < n_body; i++) {", 1)
        self.emit("        self().visit_stmt(*m_body[i]);", 1)
        self.emit("    }", 1)
        self.emit("    }")
        super(CallReplacerOnExpressionsVisitor, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(CallReplacerOnExpressionsVisitor, self).visitType(tp, tp.name)

    def visitProduct(self, prod, name):
        self.make_visitor(name, prod.fields)

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        self.emit("void visit_%s(const %s_t &x) {" % (name, name), 1)
        is_symtab_present = False
        is_stmt_present = False
        symtab_field_name = ""
        for field in fields:
            if field.type == "stmt":
                is_stmt_present = True
            if field.type == "symbol_table":
                is_symtab_present = True
                symtab_field_name = field.name
            if is_stmt_present and is_symtab_present:
                break
        if is_stmt_present and name not in ("Assignment", "ForAllSingle"):
            self.emit("    %s_t& xx = const_cast<%s_t&>(x);" % (name, name), 1)
        self.used = False

        if is_symtab_present:
            self.emit("SymbolTable* current_scope_copy = current_scope;", 2)
            self.emit("current_scope = x.m_%s;" % symtab_field_name, 2)

        for field in fields:
            self.visitField(field)
        if not self.used:
            # Note: a better solution would be to change `&x` to `& /* x */`
            # above, but we would need to change emit to return a string.
            self.emit("if ((bool&)x) { } // Suppress unused warning", 2)

        if is_symtab_present:
            self.emit("current_scope = current_scope_copy;", 2)
        self.emit("}", 1)

    def insert_call_replacer_code(self, name, level, index=""):
        self.emit("    ASR::expr_t** current_expr_copy_%d = current_expr;" % (self.current_expr_copy_variable_count), level)
        self.emit("    current_expr = const_cast<ASR::expr_t**>(&(x.m_%s%s));" % (name, index), level)
        self.emit("    self().call_replacer();", level)
        self.emit("    current_expr = current_expr_copy_%d;" % (self.current_expr_copy_variable_count), level)
        self.current_expr_copy_variable_count += 1

    def visitField(self, field):
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            level = 2
            if field.seq:
                if field.type == "stmt":
                    self.emit("self().transform_stmts(xx.m_%s, xx.n_%s);" % (field.name, field.name), level)
                    return
                self.used = True
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                if field.type in products:
                    if field.type == "expr":
                        self.insert_call_replacer_code(field.name, level, "[i]")
                    self.emit("    self().visit_%s(x.m_%s[i]);" % (field.type, field.name), level)
                else:
                    if field.type != "symbol":
                        if field.type == "expr":
                            self.insert_call_replacer_code(field.name, level, "[i]")
                        self.emit("    self().visit_%s(*x.m_%s[i]);" % (field.type, field.name), level)
                self.emit("}", level)
            else:
                if field.type in products:
                    self.used = True
                    if field.opt:
                        self.emit("if (x.m_%s) {" % field.name, 2)
                        level = 3
                        if field.type == "expr":
                            self.insert_call_replacer_code(field.name, level)
                    if field.opt:
                        self.emit("self().visit_%s(*x.m_%s);" % (field.type, field.name), level)
                        self.emit("}", 2)
                    else:
                        self.emit("self().visit_%s(x.m_%s);" % (field.type, field.name), level)
                else:
                    if field.type != "symbol":
                        self.used = True
                        if field.opt:
                            self.emit("if (x.m_%s) {" % field.name, 2)
                            level = 3
                        if field.type == "expr":
                            self.insert_call_replacer_code(field.name, level)
                        self.emit("self().visit_%s(*x.m_%s);" % (field.type, field.name), level)
                        if field.opt:
                            self.emit("}", 2)
        elif field.type == "symbol_table" and field.name in["symtab",
                "global_scope"]:
            self.used = True
            self.emit("for (auto &a : x.m_%s->get_scope()) {" % field.name, 2)
            self.emit(  "this->visit_symbol(*a.second);", 3)
            self.emit("}", 2)

# This class generates a visitor that prints the tree structure of AST/ASR
class TreeVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Tree Visitor base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class TreeBaseVisitor : public BaseVisitor<Struct>")
        self.emit("{")
        self.emit("private:")
        self.emit(  "Struct& self() { return static_cast<Struct&>(*this); }", 1)
        self.emit("public:")
        self.emit(  "std::string s, indtd;", 1)
        self.emit(  "bool use_colors;", 1)
        self.emit(  "bool start_line = true;", 1)
        self.emit(  'bool last, attached;', 1)
        self.emit(  "int indent_level = 0, indent_spaces = 2, lvl = 0;", 1)
        self.emit("public:")
        self.emit(  "TreeBaseVisitor() : use_colors(false), last(true), attached(false) { s.reserve(100000); }", 1)
        self.emit(  "void inc_indent() {", 1)
        self.emit(      "indent_level++;", 2)
        self.emit(      'indtd += "  ";', 2)
        self.emit(  "}", 1)
        self.emit(  "void inc_lindent() {", 1)
        self.emit(      "indent_level++;", 2)
        self.emit(      'indtd += "| ";', 2)
        self.emit(  "}", 1)
        self.emit(  "void dec_indent() {", 1)
        self.emit(      "indent_level--;", 2)
        self.emit(      "LCOMPILERS_ASSERT(indent_level >= 0);", 2)
        self.emit(      "indtd = indtd.substr(0, indent_level*indent_spaces);",2)
        self.emit(  "}", 1)
        self.mod = mod
        super(TreeVisitorVisitor, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        super(TreeVisitorVisitor, self).visitType(tp, tp.name)

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
        self.emit(          'if(!attached) {', 2)
        self.emit(              'if(start_line) {', 3)
        self.emit(                  'start_line = false;', 4)
        self.emit(                  's.append(indtd);', 4)
        self.emit(              '} else {', 3)
        self.emit(                  's.append("\\n"+indtd);', 4)
        self.emit(              '}', 3)
        self.emit(              'last ? s.append("└-") : s.append("|-");', 3)
        self.emit(          '}', 2)
        self.emit(          'last ? inc_indent() : inc_lindent();', 2)
        self.emit(          'attached = true;', 2)
        self.emit(          'last = false;', 2)
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
        self.used = False
        for n, field in enumerate(fields):
            self.visitField(field, cons, n == len(fields)-1)
        self.emit(    'dec_indent();', 2)
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
            self.emit(    'case (%s::%s) : {' % (name, tp.name), 3)
            self.emit(      's.append("%s");' % (tp.name), 4)
            self.emit(     ' break; }',3)
        self.emit(    '}', 2)
        self.emit(    'if (use_colors) {', 2)
        self.emit(        's.append(color(fg::reset));', 3)
        self.emit(        's.append(color(style::reset));', 3)
        self.emit(    '}', 2)
        self.emit("}", 1)

    def visitField(self, field, cons, last):
        arr = '└-' if last else '|-'
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            self.used = True
            level = 2
            if field.type in products:
                if field.opt:
                    template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
                else:
                    template = "self().visit_%s(x.m_%s);" % (field.type, field.name)
            else:
                template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
            if field.seq:
                self.emit('s.append("\\n" + indtd + "%s" + "%s=\u21a7");' % (arr, field.name), level)
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                self.emit(  'inc_indent();' if last else 'inc_lindent();', level+1)
                self.emit(  'last = i == x.n_%s-1;' % field.name, level+1)
                self.emit(  'attached = false;', level+1)
                if field.type in sums:
                    self.emit("self().visit_%s(*x.m_%s[i]);" % (field.type, field.name), level+1)
                else:
                    self.emit("self().visit_%s(x.m_%s[i]);" % (field.type, field.name), level+1)
                self.emit(  'dec_indent();', level+1)
                self.emit("}", level)
            elif field.opt:
                self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                if last:
                    self.emit('last = true;', 2)
                self.emit("if (x.m_%s) {" % field.name, 2)
                self.emit(template, 3)
                self.emit("} else {", 2)
                self.emit(    's.append("()");', 3)
                self.emit(    'last = false;', 2)
                self.emit(    'attached = false;', 2)
                self.emit("}", 2)
            else:
                self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), level)
                if last:
                    self.emit('last = true;', level)
                self.emit('attached = true;', level)
                self.emit(template, level)
        else:
            if field.type == "identifier":
                if field.seq:
                    assert not field.opt
                    level = 2
                    self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), level)
                    self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                    self.emit(  "s.append(x.m_%s[i]);" % (field.name), level+1)
                    self.emit(  'if (i < x.n_%s-1) s.append(" ");' % (field.name), level+1)
                    self.emit("}", level)
                else:
                    if field.opt:
                        self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                        self.emit("if (x.m_%s) {" % field.name, 2)
                        self.emit(    's.append(x.m_%s);' % field.name, 3)
                        self.emit("} else {", 2)
                        self.emit(    's.append("()");', 3)
                        self.emit("}", 2)
                    else:
                        self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                        self.emit('s.append(x.m_%s);' % field.name, 2)
            elif field.type == "node":
                assert not field.opt
                assert field.seq
                level = 2
                self.emit('s.append("\\n" + indtd + "%s" + "%s=\u21a7");' % (arr, field.name), level)
                self.emit('attached = false;', level)
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                mod_name = self.mod.name.lower()
                self.emit(  'inc_indent();' if last else 'inc_lindent();', level+1)
                self.emit(  'last = i == x.n_%s-1;' % field.name, level+1)
                self.emit(  'attached = false;', level+1)
                self.emit(  "self().visit_%s(*x.m_%s[i]);" % (mod_name, field.name), level+1)
                self.emit(  'dec_indent();', level+1)
                self.emit("}", level)
            elif field.type == "symbol_table":
                assert not field.opt
                assert not field.seq
                if field.name == "parent_symtab":
                    level = 2
                    self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), level)
                    self.emit('s.append(x.m_%s->get_counter());' % field.name, level)
                else:
                    level = 2
                    self.emit('s.append("\\n" + indtd + "%s");' % arr, level)
                    self.emit('inc_lindent();', level)
                    self.emit('if (use_colors) {', level)
                    self.emit(    's.append(color(fg::yellow));', level+1)
                    self.emit('}', level)
                    self.emit('s.append("SymbolTable");', level)
                    self.emit('if (use_colors) {', level)
                    self.emit(    's.append(color(fg::reset));', level+1)
                    self.emit('}', level)
                    self.emit(      's.append("\\n" + indtd + "|-counter=");', level)
                    self.emit(      's.append(x.m_%s->get_counter());' % field.name, level)
                    self.emit('size_t i = 0;', level)
                    self.emit('s.append("\\n" + indtd + "└-scope=\u21a7");', level)
                    self.emit('for (auto &a : x.m_%s->get_scope()) {' % field.name, level)
                    self.emit(      'i++;', level+1)
                    self.emit(      'inc_indent();', level+1)
                    self.emit(      'last = i == x.m_%s->get_scope().size();' % field.name, level+1)
                    self.emit(      's.append("\\n" + indtd + (last ? "└-" : "|-") + a.first + ": ");', level+1)
                    self.emit(      'this->visit_symbol(*a.second);', level+1)
                    self.emit(      'dec_indent();', level+1)
                    self.emit('}', level)
                    self.emit('dec_indent();', level)
            elif field.type == "string" and not field.seq:
                if field.opt:
                    self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    's.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    's.append("()");', 3)
                    self.emit("}", 2)
                else:
                    self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                    self.emit('s.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 2)
            elif field.type == "int" and not field.seq:
                if field.opt:
                    self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    's.append(std::to_string(x.m_%s));' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    's.append("()");', 3)
                    self.emit("}", 2)
                else:
                    self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                    self.emit('s.append(std::to_string(x.m_%s));' % field.name, 2)
            elif field.type == "float" and not field.seq and not field.opt:
                self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                self.emit('s.append(std::to_string(x.m_%s));' % field.name, 2)
            elif field.type == "bool" and not field.seq and not field.opt:
                self.emit('s.append("\\n" + indtd + "%s" + "%s=");' % (arr, field.name), 2)
                self.emit("if (x.m_%s) {" % field.name, 2)
                self.emit(    's.append(".true.");', 3)
                self.emit("} else {", 2)
                self.emit(    's.append(".false.");', 3)
                self.emit("}", 2)
            elif field.type in self.data.simple_types:
                if field.opt:
                    self.emit('s.append("Unimplementedopt");', 2)
                else:
                    self.emit('s.append("\\n" + indtd + "%s" + "%sType=");' % (arr, field.type), 2)
                    self.emit('visit_%sType(x.m_%s);' \
                            % (field.type, field.name), 2)
            else:
                self.emit('s.append("Unimplemented' + field.type + '");', 2)


class ExprStmtDuplicatorVisitor(ASDLVisitor):

    def __init__(self, stream, data):
        self.duplicate_stmt = []
        self.duplicate_expr = []
        self.duplicate_ttype = []
        self.duplicate_case_stmt = []
        self.is_stmt = False
        self.is_expr = False
        self.is_ttype = False
        self.is_case_stmt = False
        self.is_product = False
        super(ExprStmtDuplicatorVisitor, self).__init__(stream, data)

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Expression and statement Duplicator class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class BaseExprStmtDuplicator {")
        self.emit("public:")
        self.emit("    Struct& self() { return static_cast<Struct&>(*this); }")
        self.emit("")
        self.emit("    Allocator &al;")
        self.emit("    bool success;")
        self.emit("    bool allow_procedure_calls;")
        self.emit("    bool allow_reshape;")
        self.emit("")
        self.emit("    BaseExprStmtDuplicator(Allocator& al_) : al(al_), success(false), allow_procedure_calls(true), allow_reshape(true) {}")
        self.emit("")
        self.duplicate_stmt.append(("    ASR::stmt_t* duplicate_stmt(ASR::stmt_t* x) {", 0))
        self.duplicate_stmt.append(("    if( !x ) {", 1))
        self.duplicate_stmt.append(("    return nullptr;", 2))
        self.duplicate_stmt.append(("    }", 1))
        self.duplicate_stmt.append(("", 0))
        self.duplicate_stmt.append(("    switch(x->type) {", 1))

        self.duplicate_expr.append(("    ASR::expr_t* duplicate_expr(ASR::expr_t* x) {", 0))
        self.duplicate_expr.append(("    if( !x ) {", 1))
        self.duplicate_expr.append(("    return nullptr;", 2))
        self.duplicate_expr.append(("    }", 1))
        self.duplicate_expr.append(("", 0))
        self.duplicate_expr.append(("    switch(x->type) {", 1))

        self.duplicate_ttype.append(("    ASR::ttype_t* duplicate_ttype(ASR::ttype_t* x) {", 0))
        self.duplicate_ttype.append(("    if( !x ) {", 1))
        self.duplicate_ttype.append(("    return nullptr;", 2))
        self.duplicate_ttype.append(("    }", 1))
        self.duplicate_ttype.append(("", 0))
        self.duplicate_ttype.append(("    switch(x->type) {", 1))

        self.duplicate_case_stmt.append(("    ASR::case_stmt_t* duplicate_case_stmt(ASR::case_stmt_t* x) {", 0))
        self.duplicate_case_stmt.append(("    if( !x ) {", 1))
        self.duplicate_case_stmt.append(("    return nullptr;", 2))
        self.duplicate_case_stmt.append(("    }", 1))
        self.duplicate_case_stmt.append(("", 0))
        self.duplicate_case_stmt.append(("    switch(x->type) {", 1))

        super(ExprStmtDuplicatorVisitor, self).visitModule(mod)
        self.duplicate_stmt.append(("    default: {", 2))
        self.duplicate_stmt.append(('    LCOMPILERS_ASSERT_MSG(false, "Duplication of " + std::to_string(x->type) + " statement is not supported yet.");', 3))
        self.duplicate_stmt.append(("    }", 2))
        self.duplicate_stmt.append(("    }", 1))
        self.duplicate_stmt.append(("", 0))
        self.duplicate_stmt.append(("    return nullptr;", 1))
        self.duplicate_stmt.append(("    }", 0))

        self.duplicate_expr.append(("    default: {", 2))
        self.duplicate_expr.append(('    LCOMPILERS_ASSERT_MSG(false, "Duplication of " + std::to_string(x->type) + " expression is not supported yet.");', 3))
        self.duplicate_expr.append(("    }", 2))
        self.duplicate_expr.append(("    }", 1))
        self.duplicate_expr.append(("", 0))
        self.duplicate_expr.append(("    return nullptr;", 1))
        self.duplicate_expr.append(("    }", 0))

        self.duplicate_ttype.append(("    default: {", 2))
        self.duplicate_ttype.append(('    LCOMPILERS_ASSERT_MSG(false, "Duplication of " + std::to_string(x->type) + " type is not supported yet.");', 3))
        self.duplicate_ttype.append(("    }", 2))
        self.duplicate_ttype.append(("    }", 1))
        self.duplicate_ttype.append(("", 0))
        self.duplicate_ttype.append(("    return nullptr;", 1))
        self.duplicate_ttype.append(("    }", 0))

        self.duplicate_case_stmt.append(("    default: {", 2))
        self.duplicate_case_stmt.append(('    LCOMPILERS_ASSERT_MSG(false, "Duplication of " + std::to_string(x->type) + " case statement is not supported yet.");', 3))
        self.duplicate_case_stmt.append(("    }", 2))
        self.duplicate_case_stmt.append(("    }", 1))
        self.duplicate_case_stmt.append(("", 0))
        self.duplicate_case_stmt.append(("    return nullptr;", 1))
        self.duplicate_case_stmt.append(("    }", 0))

        for line, level in self.duplicate_stmt:
            self.emit(line, level=level)
        self.emit("")
        for line, level in self.duplicate_expr:
            self.emit(line, level=level)
        self.emit("")
        for line, level in self.duplicate_ttype:
            self.emit(line, level=level)
        self.emit("")
        for line, level in self.duplicate_case_stmt:
            self.emit(line, level=level)
        self.emit("")
        self.emit("};")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(ExprStmtDuplicatorVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        self.is_stmt = args[0] == 'stmt'
        self.is_expr = args[0] == 'expr'
        self.is_ttype = args[0] == "ttype"
        self.is_case_stmt = args[0] == 'case_stmt'
        if self.is_stmt or self.is_expr or self.is_case_stmt or self.is_ttype:
            for tp in sum.types:
                self.visit(tp, *args)

    def visitProduct(self, prod, name):
        pass

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        self.emit("")
        self.emit("ASR::asr_t* duplicate_%s(%s_t* x) {" % (name, name), 1)
        self.used = False
        arguments = []
        for field in fields:
            ret_value = self.visitField(field)
            for node_arg in ret_value:
                arguments.append(node_arg)
        if not self.used:
            self.emit("return (asr_t*)x;", 2)
        else:
            node_arg_str = ', '.join(arguments)
            self.emit("return make_%s_t(al, x->base.base.loc, %s);" %(name, node_arg_str), 2)
        if self.is_stmt:
            self.duplicate_stmt.append(("    case ASR::stmtType::%s: {" % name, 2))
            if name == "SubroutineCall":
                self.duplicate_stmt.append(("    if( !allow_procedure_calls ) {", 3))
                self.duplicate_stmt.append(("    success = false;", 4))
                self.duplicate_stmt.append(("    return nullptr;", 4))
                self.duplicate_stmt.append(("    }", 3))
            self.duplicate_stmt.append(("    return down_cast<ASR::stmt_t>(self().duplicate_%s(down_cast<ASR::%s_t>(x)));" % (name, name), 3))
            self.duplicate_stmt.append(("    }", 2))
        elif self.is_expr:
            self.duplicate_expr.append(("    case ASR::exprType::%s: {" % name, 2))
            if name == "FunctionCall":
                self.duplicate_expr.append(("    if( !allow_procedure_calls ) {", 3))
                self.duplicate_expr.append(("    success = false;", 4))
                self.duplicate_expr.append(("    return nullptr;", 4))
                self.duplicate_expr.append(("    }", 3))
            elif name == "ArrayReshape":
                self.duplicate_expr.append(("    if( !allow_reshape ) {", 3))
                self.duplicate_expr.append(("    success = false;", 4))
                self.duplicate_expr.append(("    return nullptr;", 4))
                self.duplicate_expr.append(("    }", 3))
            self.duplicate_expr.append(("    return down_cast<ASR::expr_t>(self().duplicate_%s(down_cast<ASR::%s_t>(x)));" % (name, name), 3))
            self.duplicate_expr.append(("    }", 2))
        elif self.is_ttype:
            self.duplicate_ttype.append(("    case ASR::ttypeType::%s: {" % name, 2))
            self.duplicate_ttype.append(("    return down_cast<ASR::ttype_t>(self().duplicate_%s(down_cast<ASR::%s_t>(x)));" % (name, name), 3))
            self.duplicate_ttype.append(("    }", 2))
        elif self.is_case_stmt:
            self.duplicate_case_stmt.append(("    case ASR::case_stmtType::%s: {" % name, 2))
            self.duplicate_case_stmt.append(("    return down_cast<ASR::case_stmt_t>(self().duplicate_%s(down_cast<ASR::%s_t>(x)));" % (name, name), 3))
            self.duplicate_case_stmt.append(("    }", 2))
        self.emit("}", 1)
        self.emit("")

    def visitField(self, field):
        arguments = None
        if (field.type == "expr" or
            field.type == "stmt" or
            field.type == "symbol" or
            field.type == "call_arg" or
            field.type == "do_loop_head" or
            field.type == "array_index" or
            field.type == "alloc_arg" or
            field.type == "case_stmt" or
            field.type == "ttype"):
            level = 2
            if field.seq:
                self.used = True
                pointer_char = ''
                if (field.type != "call_arg" and
                    field.type != "array_index" and
                    field.type != "alloc_arg"):
                    pointer_char = '*'
                self.emit("Vec<%s_t%s> m_%s;" % (field.type, pointer_char, field.name), level)
                self.emit("m_%s.reserve(al, x->n_%s);" % (field.name, field.name), level)
                self.emit("for (size_t i = 0; i < x->n_%s; i++) {" % field.name, level)
                if field.type == "symbol":
                    self.emit("    m_%s.push_back(al, x->m_%s[i]);" % (field.name, field.name), level)
                elif field.type == "call_arg":
                    self.emit("    ASR::call_arg_t call_arg_copy;", level)
                    self.emit("    call_arg_copy.loc = x->m_%s[i].loc;"%(field.name), level)
                    self.emit("    call_arg_copy.m_value = self().duplicate_expr(x->m_%s[i].m_value);"%(field.name), level)
                    self.emit("    m_%s.push_back(al, call_arg_copy);"%(field.name), level)
                elif field.type == "alloc_arg":
                    self.emit("    ASR::alloc_arg_t alloc_arg_copy;", level)
                    self.emit("    alloc_arg_copy.loc = x->m_%s[i].loc;"%(field.name), level)
                    self.emit("    alloc_arg_copy.m_a = x->m_%s[i].m_a;"%(field.name), level)
                    self.emit("    alloc_arg_copy.n_dims = x->m_%s[i].n_dims;"%(field.name), level)
                    self.emit("    Vec<ASR::dimension_t> dims_copy;", level)
                    self.emit("    dims_copy.reserve(al, alloc_arg_copy.n_dims);", level)
                    self.emit("    for (size_t j = 0; j < alloc_arg_copy.n_dims; j++) {", level)
                    self.emit("    ASR::dimension_t dim_copy;", level + 1)
                    self.emit("    dim_copy.loc = x->m_%s[i].m_dims[j].loc;"%(field.name), level + 1)
                    self.emit("    dim_copy.m_start = self().duplicate_expr(x->m_%s[i].m_dims[j].m_start);"%(field.name), level + 1)
                    self.emit("    dim_copy.m_length = self().duplicate_expr(x->m_%s[i].m_dims[j].m_length);"%(field.name), level + 1)
                    self.emit("    dims_copy.push_back(al, dim_copy);", level + 1)
                    self.emit("    }", level)
                    self.emit("    alloc_arg_copy.m_dims = dims_copy.p;", level)
                    self.emit("    m_%s.push_back(al, alloc_arg_copy);"%(field.name), level)
                elif field.type == "array_index":
                    self.emit("    ASR::array_index_t array_index_copy;", level)
                    self.emit("    array_index_copy.loc = x->m_%s[i].loc;"%(field.name), level)
                    self.emit("    array_index_copy.m_left = duplicate_expr(x->m_%s[i].m_left);"%(field.name), level)
                    self.emit("    array_index_copy.m_right = duplicate_expr(x->m_%s[i].m_right);"%(field.name), level)
                    self.emit("    array_index_copy.m_step = duplicate_expr(x->m_%s[i].m_step);"%(field.name), level)
                    self.emit("    m_%s.push_back(al, array_index_copy);"%(field.name), level)
                else:
                    self.emit("    m_%s.push_back(al, self().duplicate_%s(x->m_%s[i]));" % (field.name, field.type, field.name), level)
                self.emit("}", level)
                arguments = ("m_" + field.name + ".p", "x->n_" + field.name)
            else:
                self.used = True
                if field.type == "symbol":
                    self.emit("%s_t* m_%s = x->m_%s;" % (field.type, field.name, field.name), level)
                elif field.type == "do_loop_head":
                    self.emit("ASR::do_loop_head_t m_head;", level)
                    self.emit("m_head.loc = x->m_head.loc;", level)
                    self.emit("m_head.m_v = duplicate_expr(x->m_head.m_v);", level)
                    self.emit("m_head.m_start = duplicate_expr(x->m_head.m_start);", level)
                    self.emit("m_head.m_end = duplicate_expr(x->m_head.m_end);", level)
                    self.emit("m_head.m_increment = duplicate_expr(x->m_head.m_increment);", level)
                elif field.type == "array_index":
                    self.emit("ASR::array_index_t m_%s;"%(field.name), level)
                    self.emit("m_%s.loc = x->m_%s.loc;"%(field.name, field.name), level)
                    self.emit("m_%s.m_left = duplicate_expr(x->m_%s.m_left);"%(field.name, field.name), level)
                    self.emit("m_%s.m_right = duplicate_expr(x->m_%s.m_right);"%(field.name, field.name), level)
                    self.emit("m_%s.m_step = duplicate_expr(x->m_%s.m_step);"%(field.name, field.name), level)
                else:
                    self.emit("%s_t* m_%s = self().duplicate_%s(x->m_%s);" % (field.type, field.name, field.type, field.name), level)
                arguments = ("m_" + field.name, )
        else:
            if field.seq:
                arguments = ("x->m_" + field.name, "x->n_" + field.name)
            else:
                arguments = ("x->m_" + field.name, )
        return arguments

class ExprBaseReplacerVisitor(ASDLVisitor):

    def __init__(self, stream, data):
        self.replace_expr = []
        self.is_expr = False
        self.is_product = False
        self.current_expr_copy_variable_count = 0
        super(ExprBaseReplacerVisitor, self).__init__(stream, data)

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Expression Replacer Base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class BaseExprReplacer {")
        self.emit("public:")
        self.emit("    Struct& self() { return static_cast<Struct&>(*this); }")
        self.emit("")
        self.emit("    ASR::expr_t** current_expr;")
        self.emit("")
        self.emit("    BaseExprReplacer() : current_expr(nullptr) {}")
        self.emit("")

        self.replace_expr.append(("    void replace_expr(ASR::expr_t* x) {", 0))
        self.replace_expr.append(("    if( !x ) {", 1))
        self.replace_expr.append(("    return ;", 2))
        self.replace_expr.append(("    }", 1))
        self.replace_expr.append(("", 0))
        self.replace_expr.append(("    switch(x->type) {", 1))

        super(ExprBaseReplacerVisitor, self).visitModule(mod)

        self.replace_expr.append(("    default: {", 2))
        self.replace_expr.append(('    LCOMPILERS_ASSERT_MSG(false, "Duplication of " + std::to_string(x->type) + " expression is not supported yet.");', 3))
        self.replace_expr.append(("    }", 2))
        self.replace_expr.append(("    }", 1))
        self.replace_expr.append(("", 0))
        self.replace_expr.append(("    }", 0))
        for line, level in self.replace_expr:
            self.emit(line, level=level)
        self.emit("")
        self.emit("};")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(ExprBaseReplacerVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        self.is_expr = args[0] == 'expr'
        if self.is_expr:
            for tp in sum.types:
                self.visit(tp, *args)

    def visitProduct(self, prod, name):
        pass

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        self.emit("")
        self.emit("void replace_%s(%s_t* x) {" % (name, name), 1)
        self.used = False
        for field in fields:
            self.visitField(field)
        if not self.used:
            self.emit("if (x) { }", 2)

        if self.is_expr:
            self.replace_expr.append(("    case ASR::exprType::%s: {" % name, 2))
            self.replace_expr.append(("    self().replace_%s(down_cast<ASR::%s_t>(x));" % (name, name), 3))
            self.replace_expr.append(("    break;", 3))
            self.replace_expr.append(("    }", 2))
        self.emit("}", 1)
        self.emit("")

    def visitField(self, field):
        arguments = None
        if field.type == "expr" or field.type == "symbol" or field.type == "call_arg":
            level = 2
            if field.seq:
                self.used = True
                self.emit("for (size_t i = 0; i < x->n_%s; i++) {" % field.name, level)
                if field.type == "call_arg":
                    self.emit("    if (x->m_%s[i].m_value != nullptr) {" % (field.name), level)
                    self.emit("    ASR::expr_t** current_expr_copy_%d = current_expr;" % (self.current_expr_copy_variable_count), level + 1)
                    self.emit("    current_expr = &(x->m_%s[i].m_value);" % (field.name), level + 1)
                    self.emit("    self().replace_expr(x->m_%s[i].m_value);"%(field.name), level + 1)
                    self.emit("    current_expr = current_expr_copy_%d;" % (self.current_expr_copy_variable_count), level + 1)
                    self.emit("    }", level)
                    self.current_expr_copy_variable_count += 1
                self.emit("}", level)
            else:
                if field.type != "symbol":
                    self.used = True
                    self.emit("ASR::expr_t** current_expr_copy_%d = current_expr;" % (self.current_expr_copy_variable_count), level)
                    self.emit("current_expr = &(x->m_%s);" % (field.name), level)
                    self.emit("self().replace_%s(x->m_%s);" % (field.type, field.name), level)
                    self.emit("current_expr = current_expr_copy_%d;" % (self.current_expr_copy_variable_count), level)
                    self.current_expr_copy_variable_count += 1

class StmtBaseReplacerVisitor(ASDLVisitor):

    def __init__(self, stream, data):
        self.replace_stmt = []
        self.is_stmt = False
        self.is_product = False
        super(StmtBaseReplacerVisitor, self).__init__(stream, data)

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Statement Replacer Base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class BaseStmtReplacer {")
        self.emit("public:")
        self.emit("    Struct& self() { return static_cast<Struct&>(*this); }")
        self.emit("")
        self.emit("    ASR::stmt_t** current_stmt;")
        self.emit("    ASR::stmt_t** current_stmt_copy;")
        self.emit("    bool has_replacement_happened;")
        self.emit("")
        self.emit("    BaseStmtReplacer() : current_stmt(nullptr), has_replacement_happened(false) {}")
        self.emit("")

        self.replace_stmt.append(("    void replace_stmt(ASR::stmt_t* x) {", 0))
        self.replace_stmt.append(("    if( !x ) {", 1))
        self.replace_stmt.append(("    return ;", 2))
        self.replace_stmt.append(("    }", 1))
        self.replace_stmt.append(("", 0))
        self.replace_stmt.append(("    switch(x->type) {", 1))

        super(StmtBaseReplacerVisitor, self).visitModule(mod)

        self.replace_stmt.append(("    default: {", 2))
        self.replace_stmt.append(('    LCOMPILERS_ASSERT_MSG(false, "Replacement of " + std::to_string(x->type) + " statement is not supported yet.");', 3))
        self.replace_stmt.append(("    }", 2))
        self.replace_stmt.append(("    }", 1))
        self.replace_stmt.append(("", 0))
        self.replace_stmt.append(("    }", 0))
        for line, level in self.replace_stmt:
            self.emit(line, level=level)
        self.emit("")
        self.emit("};")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(StmtBaseReplacerVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        self.is_stmt = args[0] == 'stmt'
        if self.is_stmt:
            for tp in sum.types:
                self.visit(tp, *args)

    def visitProduct(self, prod, name):
        pass

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        self.emit("")
        self.emit("void replace_%s(%s_t* x) {" % (name, name), 1)
        self.used = False
        for field in fields:
            self.visitField(field)
        if not self.used:
            self.emit("if (x) { }", 2)

        if self.is_stmt:
            self.replace_stmt.append(("    case ASR::stmtType::%s: {" % name, 2))
            self.replace_stmt.append(("    self().replace_%s(down_cast<ASR::%s_t>(x));" % (name, name), 3))
            self.replace_stmt.append(("    break;", 3))
            self.replace_stmt.append(("    }", 2))
        self.emit("}", 1)
        self.emit("")

    def visitField(self, field):
        arguments = None
        if field.type == "stmt":
            level = 2
            if field.seq:
                self.used = True
                self.emit("for (size_t i = 0; i < x->n_%s; i++) {" % field.name, level)
                self.emit("    current_stmt_copy = current_stmt;", level)
                self.emit("    current_stmt = &(x->m_%s[i]);" % (field.name), level)
                self.emit("    self().replace_stmt(x->m_%s[i]);"%(field.name), level)
                self.emit("    current_stmt = current_stmt_copy;", level)
                self.emit("}", level)

class PickleVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Pickle Visitor base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class PickleBaseVisitor : public BaseVisitor<Struct>")
        self.emit("{")
        self.emit("private:")
        self.emit(  "Struct& self() { return static_cast<Struct&>(*this); }", 1)
        self.emit("public:")
        self.emit(  "std::string s, indented = \"\";", 1)
        self.emit(  "bool use_colors;", 1)
        self.emit(  "bool indent;", 1)
        self.emit(  "int indent_level = 0, indent_spaces = 4;", 1)
        self.emit("public:")
        self.emit(  "PickleBaseVisitor() : use_colors(false), indent(false) { s.reserve(100000); }", 1)
        self.emit(  "void inc_indent() {", 1)
        self.emit(      "indent_level++;", 2)
        self.emit(      "indented = std::string(indent_level*indent_spaces, ' ');",2)
        self.emit(  "}",1)
        self.emit(  "void dec_indent() {", 1)
        self.emit(      "indent_level--;", 2)
        self.emit(      "LCOMPILERS_ASSERT(indent_level >= 0);", 2)
        self.emit(      "indented = std::string(indent_level*indent_spaces, ' ');",2)
        self.emit(  "}",1)
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
        self.emit(      's.append("(");', 2)
        subs = {
            "Assignment": "=",
            "Associate": "=>",
        }
        if name in subs:
            name = subs[name]

        # For ASR
        symbol = [
            "Integer",
            "Real",
            "Complex",
            "Character",
            "Logical",
            "Var",
        ]

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
                if name not in symbol:
                    self.emit(    'if(indent) {', 2)
                    self.emit(        'inc_indent();', 3)
                    self.emit(        's.append("\\n" + indented);', 3)
                    self.emit(    '} else {', 2)
                    self.emit(        's.append(" ");', 3)
                    self.emit(    '}', 2)
                else:
                    self.emit(    's.append(" ");', 2)
        self.used = False
        for n, field in enumerate(fields):
            self.visitField(field, cons)
            if n < len(fields) - 1:
                if name not in symbol:
                    self.emit(    'if(indent) s.append("\\n" + indented);', 2)
                    self.emit(    'else s.append(" ");', 2)
                else:
                    self.emit(    's.append(" ");', 2)
        if name not in symbol and cons and len(fields) > 0:
            self.emit(    'if(indent) {', 2)
            self.emit(        'dec_indent();', 3)
            self.emit(        's.append("\\n" + indented);', 3)
            self.emit(    '}', 2)
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
            self.emit(    'case (%s::%s) : {' % (name, tp.name), 3)
            self.emit(      's.append("%s");' % (tp.name), 4)
            self.emit(     ' break; }',3)
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
                    template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
                else:
                    template = "self().visit_%s(x.m_%s);" % (field.type, field.name)
            else:
                template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
            if field.seq:
                self.emit('s.append("[");', level)
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                if field.type in sums:
                    self.emit("self().visit_%s(*x.m_%s[i]);" % (field.type, field.name), level+1)
                else:
                    self.emit("self().visit_%s(x.m_%s[i]);" % (field.type, field.name), level+1)
                self.emit('    if (i < x.n_%s-1) {' % (field.name), level)
                self.emit('        if (indent) s.append("\\n" + indented);', level)
                self.emit('        else s.append(" ");', level)
                self.emit('    };', level)
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
                    self.emit("    s.append(x.m_%s[i]);" % (field.name), level)
                    self.emit('    if (i < x.n_%s-1) {' % (field.name), level)
                    self.emit('        if (indent) s.append("\\n" + indented);', level)
                    self.emit('        else s.append(" ");', level)
                    self.emit('    };', level)
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
                self.emit("    self().visit_%s(*x.m_%s[i]);" % (mod_name, field.name), level)
                self.emit('    if (i < x.n_%s-1) {' % (field.name), level)
                self.emit('        if (indent) s.append("\\n" + indented);', level)
                self.emit('        else s.append(" ");', level)
                self.emit('    };', level)
                self.emit("}", level)
                self.emit('s.append("]");', level)
            elif field.type == "symbol_table":
                assert not field.opt
                assert not field.seq
                if field.name == "parent_symtab":
                    level = 2
                    self.emit('s.append(x.m_%s->get_counter());' % field.name, level)
                else:
                    level = 2
                    self.emit(      's.append("(");', level)
                    self.emit('if (use_colors) {', level)
                    self.emit(    's.append(color(fg::yellow));', level+1)
                    self.emit('}', level)
                    self.emit('s.append("SymbolTable");', level)
                    self.emit('if (use_colors) {', level)
                    self.emit(    's.append(color(fg::reset));', level+1)
                    self.emit('}', level)
                    self.emit('if(indent) {', level)
                    self.emit('    inc_indent();', level)
                    self.emit('    s.append("\\n" + indented);', level)
                    self.emit('} else {', level)
                    self.emit('    s.append(" ");', level)
                    self.emit('}', level)
                    self.emit('s.append(x.m_%s->get_counter());' % field.name, level)
                    self.emit('if(indent) s.append("\\n" + indented);', level)
                    self.emit('else s.append(" ");', level)
                    self.emit(      's.append("{");', level)
                    self.emit('if(indent) {', level)
                    self.emit('    inc_indent();', level)
                    self.emit('    s.append("\\n" + indented);', level)
                    self.emit('}', level)
                    self.emit('{', level)
                    self.emit('    size_t i = 0;', level)
                    self.emit('    for (auto &a : x.m_%s->get_scope()) {' % field.name, level)
                    self.emit('        s.append(a.first + ":");', level)
                    self.emit('        if(indent) {', level)
                    self.emit('            inc_indent();', level)
                    self.emit('            s.append("\\n" + indented);', level)
                    self.emit('        } else {', level)
                    self.emit('            s.append(" ");', level)
                    self.emit('        }', level)
                    self.emit('        this->visit_symbol(*a.second);', level)
                    self.emit('        if(indent) dec_indent();', level)
                    self.emit('        if (i < x.m_%s->get_scope().size()-1) {' % field.name, level)
                    self.emit('            s.append(",");', level)
                    self.emit('            if(indent) s.append("\\n" + indented);', level)
                    self.emit('            else s.append(" ");', level)
                    self.emit('        }', level)
                    self.emit('        i++;', level)
                    self.emit('    }', level)
                    self.emit('}', level)
                    self.emit('if(indent) {', level)
                    self.emit(    'dec_indent();', level+1)
                    self.emit(    's.append("\\n" + indented);', level+1)
                    self.emit('}', level)
                    self.emit('s.append("})");', level)
                    self.emit('if(indent) dec_indent();', level)
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
            elif field.type == "float" and not field.seq and not field.opt:
                self.emit('s.append(std::to_string(x.m_%s));' % field.name, 2)
            elif field.type == "bool" and not field.seq and not field.opt:
                self.emit("if (x.m_%s) {" % field.name, 2)
                self.emit(    's.append(".true.");', 3)
                self.emit("} else {", 2)
                self.emit(    's.append(".false.");', 3)
                self.emit("}", 2)
            elif field.type in self.data.simple_types:
                if field.opt:
                    self.emit('s.append("Unimplementedopt");', 2)
                else:
                    self.emit('visit_%sType(x.m_%s);' \
                            % (field.type, field.name), 2)
            else:
                self.emit('s.append("Unimplemented' + field.type + '");', 2)

class JsonVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Json Visitor base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class JsonBaseVisitor : public BaseVisitor<Struct>")
        self.emit("{")
        self.emit("private:")
        self.emit(  "Struct& self() { return static_cast<Struct&>(*this); }", 1)
        self.emit("public:")
        self.emit(  "std::string s, indtd = \"\";", 1)
        self.emit(  "int indent_level = 0, indent_spaces = 4;", 1)
        # Storing a reference to LocationManager like this isn't ideal.
        # One must make sure JsonBaseVisitor isn't reused in a case where AST/ASR has changed
        # but lm wasn't updated correspondingly.
        # If LocationManager becomes needed in any of the other visitors, it should be
        # passed by reference into all the visit functions instead of storing the reference here.
        self.emit(  "LocationManager &lm;", 1)
        self.emit("public:")
        self.emit(  "JsonBaseVisitor(LocationManager &lmref) : lm(lmref) {", 1);
        self.emit(     "s.reserve(100000);", 2)
        self.emit(  "}", 1)
        self.emit(  "void inc_indent() {", 1)
        self.emit(      "indent_level++;", 2)
        self.emit(      "indtd = std::string(indent_level*indent_spaces, ' ');",2)
        self.emit(  "}",1)
        self.emit(  "void dec_indent() {", 1)
        self.emit(      "indent_level--;", 2)
        self.emit(      "LCOMPILERS_ASSERT(indent_level >= 0);", 2)
        self.emit(      "indtd = std::string(indent_level*indent_spaces, ' ');",2)
        self.emit(  "}",1)
        self.emit(  "void append_location(std::string &s, uint32_t first, uint32_t last) {", 1)
        self.emit(      's.append("\\"loc\\": {");', 2);
        self.emit(      'inc_indent();', 2)
        self.emit(      's.append("\\n" + indtd);', 2)
        self.emit(      's.append("\\"first\\": " + std::to_string(first));', 2)
        self.emit(      's.append(",\\n" + indtd);', 2)
        self.emit(      's.append("\\"last\\": " + std::to_string(last));', 2)
        self.emit(      '')
        self.emit(      'uint32_t first_line = 0, first_col = 0;', 2)
        self.emit(      'std::string first_filename;', 2)
        self.emit(      'uint32_t last_line = 0, last_col = 0;', 2)
        self.emit(      'std::string last_filename;', 2)
        self.emit(      '')
        self.emit(      'lm.pos_to_linecol(first, first_line, first_col, first_filename);', 2)
        self.emit(      'lm.pos_to_linecol(last, last_line, last_col, last_filename);', 2)
        self.emit(      '')
        self.emit(      's.append(",\\n" + indtd);', 2)
        self.emit(      's.append("\\"first_filename\\": \\"" + first_filename + "\\"");', 2)
        self.emit(      's.append(",\\n" + indtd);', 2)
        self.emit(      's.append("\\"first_line\\": " + std::to_string(first_line));', 2)
        self.emit(      's.append(",\\n" + indtd);', 2)
        self.emit(      's.append("\\"first_column\\": " + std::to_string(first_col));', 2)
        self.emit(      's.append(",\\n" + indtd);', 2)
        self.emit(      's.append("\\"last_filename\\": \\"" + last_filename + "\\"");', 2)
        self.emit(      's.append(",\\n" + indtd);', 2)
        self.emit(      's.append("\\"last_line\\": " + std::to_string(last_line));', 2)
        self.emit(      's.append(",\\n" + indtd);', 2)
        self.emit(      's.append("\\"last_column\\": " + std::to_string(last_col));', 2)
        self.emit(      '')
        self.emit(      'dec_indent();', 2)
        self.emit(      's.append("\\n" + indtd);', 2)
        self.emit(      's.append("}");', 2)
        self.emit(  '}', 1)

        self.mod = mod
        super(JsonVisitorVisitor, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        super(JsonVisitorVisitor, self).visitType(tp, tp.name)

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
        self.emit(    's.append("{");', 2)
        self.emit(    'inc_indent(); s.append("\\n" + indtd);', 2)
        self.emit(    's.append("\\"node\\": \\"%s\\"");' % name, 2)
        self.emit(    's.append(",\\n" + indtd);', 2)
        self.emit(    's.append("\\"fields\\": {");', 2)
        if len(fields) > 0:
            self.emit('inc_indent(); s.append("\\n" + indtd);', 2)
            for n, field in enumerate(fields):
                self.visitField(field, cons)
                if n < len(fields) - 1:
                    self.emit('s.append(",\\n" + indtd);', 2)
            self.emit('dec_indent(); s.append("\\n" + indtd);', 2)
        self.emit(    's.append("}");', 2)
        self.emit(    's.append(",\\n" + indtd);', 2)
        if name in products:
            self.emit(    'append_location(s, x.loc.first, x.loc.last);', 2)
        else:
            self.emit(    'append_location(s, x.base.base.loc.first, x.base.base.loc.last);', 2)

        self.emit(    'dec_indent(); s.append("\\n" + indtd);', 2)
        self.emit(    's.append("}");', 2)
        self.emit(    'if ((bool&)x) { } // Suppress unused warning', 2)
        self.emit("}", 1)

    def make_simple_sum_visitor(self, name, types):
        self.emit("void visit_%s(const %s &x) {" % (name, name), 1)
        self.emit(    'switch (x) {', 2)
        for tp in types:
            self.emit(    'case (%s::%s) : {' % (name, tp.name), 3)
            self.emit(      's.append("\\"%s\\"");' % (tp.name), 4)
            self.emit(     ' break; }',3)
        self.emit(    '}', 2)
        self.emit("}", 1)

    def visitField(self, field, cons):
        self.emit('s.append("\\"%s\\": ");' % field.name, 2)
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            self.used = True
            level = 2
            if field.type in products:
                if field.opt:
                    template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
                else:
                    template = "self().visit_%s(x.m_%s);" % (field.type, field.name)
            else:
                template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
            if field.seq:
                self.emit('s.append("[");', level)
                self.emit('if (x.n_%s > 0) {' % field.name, level)
                self.emit(    'inc_indent(); s.append("\\n" + indtd);', level+1)
                self.emit(    "for (size_t i=0; i<x.n_%s; i++) {" % field.name, level+1)
                if field.type in sums:
                    self.emit(    "self().visit_%s(*x.m_%s[i]);" % (field.type, field.name), level+2)
                else:
                    self.emit(    "self().visit_%s(x.m_%s[i]);" % (field.type, field.name), level+2)
                self.emit(        'if (i < x.n_%s-1) {' % (field.name), level+2)
                self.emit(            's.append(",\\n" + indtd);', level+3)
                self.emit(        '};', level+2)
                self.emit(    "}", level+1)
                self.emit(    'dec_indent(); s.append("\\n" + indtd);', level+1)
                self.emit('}', level)
                self.emit('s.append("]");', level)
            elif field.opt:
                self.emit("if (x.m_%s) {" % field.name, level)
                self.emit(    template, level+1)
                self.emit("} else {", level)
                self.emit(    's.append("[]");', level+1)
                self.emit("}", 2)
            else:
                self.emit(template, level)
        else:
            if field.type == "identifier":
                if field.seq:
                    assert not field.opt
                    level = 2
                    self.emit('s.append("[");', level)
                    self.emit('if (x.n_%s > 0) {' % field.name, level)
                    self.emit(    'inc_indent(); s.append("\\n" + indtd);', level+1)
                    self.emit(    "for (size_t i=0; i<x.n_%s; i++) {" % field.name, level+1)
                    self.emit(        's.append("\\"" + std::string(x.m_%s[i]) + "\\"");' % (field.name), level+2)
                    self.emit(        'if (i < x.n_%s-1) {' % (field.name), level+2)
                    self.emit(            's.append(",\\n" + indtd);', level+3)
                    self.emit(        '};', level+2)
                    self.emit(    "}", level+1)
                    self.emit(    'dec_indent(); s.append("\\n" + indtd);', level+1)
                    self.emit('}', level)
                    self.emit('s.append("]");', level)
                else:
                    if field.opt:
                        self.emit("if (x.m_%s) {" % field.name, 2)
                        self.emit(    's.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 3)
                        self.emit("} else {", 2)
                        self.emit(    's.append("[]");', 3)
                        self.emit("}", 2)
                    else:
                        self.emit('s.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 2)
            elif field.type == "node":
                assert not field.opt
                assert field.seq
                level = 2
                mod_name = self.mod.name.lower()
                self.emit('s.append("[");', level)
                self.emit('if (x.n_%s > 0) {' % field.name, level)
                self.emit(    'inc_indent(); s.append("\\n" + indtd);', level+1)
                self.emit(    "for (size_t i=0; i<x.n_%s; i++) {" % field.name, level+1)
                self.emit(        "self().visit_%s(*x.m_%s[i]);" % (mod_name, field.name), level+2)
                self.emit(        'if (i < x.n_%s-1) {' % (field.name), level+2)
                self.emit(            's.append(",\\n" + indtd);', level+3)
                self.emit(        '};', level+2)
                self.emit(    "}", level+1)
                self.emit(    'dec_indent(); s.append("\\n" + indtd);', level+1)
                self.emit('}', level)
                self.emit('s.append("]");', level)
            elif field.type == "symbol_table":
                assert not field.opt
                assert not field.seq
                if field.name == "parent_symtab":
                    level = 2
                    self.emit('s.append(x.m_%s->get_counter());' % field.name, level)
                else:
                    level = 2
                    self.emit('s.append("{");', level)
                    self.emit('inc_indent(); s.append("\\n" + indtd);', level)
                    self.emit('s.append("\\"node\\": \\"SymbolTable" + x.m_%s->get_counter() +"\\"");' % field.name, level)
                    self.emit('s.append(",\\n" + indtd);', level)
                    self.emit('s.append("\\"fields\\": {");', level)
                    self.emit('if (x.m_%s->get_scope().size() > 0) {' % field.name, level)
                    self.emit(    'inc_indent(); s.append("\\n" + indtd);', level+1)
                    self.emit(    'size_t i = 0;', level+1)
                    self.emit(    'for (auto &a : x.m_%s->get_scope()) {' % field.name, level+1)
                    self.emit(        's.append("\\"" + a.first + "\\": ");', level+2)
                    self.emit(        'this->visit_symbol(*a.second);', level+2)
                    self.emit(        'if (i < x.m_%s->get_scope().size()-1) { ' % field.name, level+2)
                    self.emit(        '    s.append(",\\n" + indtd);', level+3)
                    self.emit(        '}', level+2)
                    self.emit(        'i++;', level+2)
                    self.emit(    '}', level+1)
                    self.emit(    'dec_indent(); s.append("\\n" + indtd);', level+1)
                    self.emit('}', level)
                    self.emit('s.append("}");', level)
                    self.emit('dec_indent(); s.append("\\n" + indtd);', level)
                    self.emit('s.append("}");', level)
            elif field.type == "string" and not field.seq:
                if field.opt:
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    's.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    's.append("[]");', 3)
                    self.emit("}", 2)
                else:
                    self.emit('s.append("\\"" + std::string(x.m_%s) + "\\"");' % field.name, 2)
            elif field.type == "int" and not field.seq:
                if field.opt:
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    's.append(std::to_string(x.m_%s));' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    's.append("[]");', 3)
                    self.emit("}", 2)
                else:
                    self.emit('s.append(std::to_string(x.m_%s));' % field.name, 2)
            elif field.type == "float" and not field.seq and not field.opt:
                self.emit('s.append(std::to_string(x.m_%s));' % field.name, 2)
            elif field.type == "bool" and not field.seq and not field.opt:
                self.emit("if (x.m_%s) {" % field.name, 2)
                self.emit(    's.append("true");', 3)
                self.emit("} else {", 2)
                self.emit(    's.append("false");', 3)
                self.emit("}", 2)
            elif field.type in self.data.simple_types:
                if field.opt:
                    self.emit('s.append("\\"Unimplementedopt\\"");', 2)
                else:
                    self.emit('visit_%sType(x.m_%s);' \
                            % (field.type, field.name), 2)
            else:
                self.emit('s.append("\\"Unimplemented%s\\"");' % field.type, 2)


class SerializationVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Serialization Visitor base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class SerializationBaseVisitor : public BaseVisitor<Struct>")
        self.emit("{")
        self.emit("private:")
        self.emit(  "Struct& self() { return static_cast<Struct&>(*this); }", 1)
        self.emit("public:")
        self.mod = mod
        super(SerializationVisitorVisitor, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        super(SerializationVisitorVisitor, self).visitType(tp, tp.name)

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
        if cons:
            self.emit(    'self().write_int8(x.base.type);', 2)
            self.emit(    'self().write_int64(x.base.base.loc.first);', 2)
            self.emit(    'self().write_int64(x.base.base.loc.last);', 2)
        self.used = False
        for n, field in enumerate(fields):
            self.visitField(field, cons, name)
        if not self.used:
            # Note: a better solution would be to change `&x` to `& /* x */`
            # above, but we would need to change emit to return a string.
            self.emit("if ((bool&)x) { } // Suppress unused warning", 2)
        self.emit("}", 1)

    def make_simple_sum_visitor(self, name, types):
        self.emit("void visit_%s(const %s &x) {" % (name, name), 1)
        self.emit(    'self().write_int8(x);', 2)
        self.emit("}", 1)

    def visitField(self, field, cons, cons_name):
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            self.used = True
            level = 2
            if field.type in products:
                if field.opt:
                    template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
                else:
                    template = "self().visit_%s(x.m_%s);" % (field.type, field.name)
            else:
                if field.type == "symbol":
                    if cons_name == "ExternalSymbol":
                        template = "// We skip the symbol for ExternalSymbol"
                    else:
                        template = "self().write_symbol(*x.m_%s);" \
                            % field.name
                else:
                    template = "self().visit_%s(*x.m_%s);" % (field.type, field.name)
            if field.seq:
                self.emit('self().write_int64(x.n_%s);' % field.name, level)
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                if field.type == "symbol":
                    self.emit("self().write_symbol(*x.m_%s[i]);" % (field.name), level+1)
                elif field.type in sums:
                    self.emit("self().visit_%s(*x.m_%s[i]);" % (field.type, field.name), level+1)
                else:
                    self.emit("self().visit_%s(x.m_%s[i]);" % (field.type, field.name), level+1)
                self.emit("}", level)
            elif field.opt:
                self.emit("if (x.m_%s) {" % field.name, 2)
                self.emit(    'self().write_bool(true);', 3)
                self.emit(template, 3)
                self.emit("} else {", 2)
                self.emit(    'self().write_bool(false);', 3)
                self.emit("}", 2)
            else:
                self.emit(template, level)
        else:
            if field.type == "identifier":
                if field.seq:
                    assert not field.opt
                    level = 2
                    self.emit('self().write_int64(x.n_%s);' % field.name, level)
                    self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                    self.emit("self().write_string(x.m_%s[i]);" % (field.name), level+1)
                    self.emit("}", level)
                else:
                    if field.opt:
                        self.emit("if (x.m_%s) {" % field.name, 2)
                        self.emit(    'self().write_bool(true);', 3)
                        self.emit(    'self().write_string(x.m_%s);' % field.name, 3)
                        self.emit("} else {", 2)
                        self.emit(    'self().write_bool(false);', 3)
                        self.emit("}", 2)
                    else:
                        self.emit('self().write_string(x.m_%s);' % field.name, 2)
            elif field.type == "node":
                assert not field.opt
                assert field.seq
                level = 2
                self.emit('self().write_int64(x.n_%s);' % field.name, level)
                self.emit("for (size_t i=0; i<x.n_%s; i++) {" % field.name, level)
                mod_name = self.mod.name.lower()
                self.emit("self().write_int8(x.m_%s[i]->type);" % \
                        field.name, level+1)
                self.emit("self().visit_%s(*x.m_%s[i]);" % (mod_name, field.name), level+1)
                self.emit("}", level)
            elif field.type == "symbol_table":
                assert not field.opt
                assert not field.seq
                # TODO: write the symbol table consistent with the reader:
                if field.name == "parent_symtab":
                    level = 2
                    self.emit('self().write_int64(x.m_%s->counter);' % field.name, level)
                else:
                    level = 2
                    self.emit('self().write_int64(x.m_%s->counter);' % field.name, level)
                    self.emit('self().write_int64(x.m_%s->get_scope().size());' % field.name, level)
                    self.emit('for (auto &a : x.m_%s->get_scope()) {' % field.name, level)
                    self.emit('    if (ASR::is_a<ASR::Function_t>(*a.second)) {', level)
                    self.emit('        continue;', level)
                    self.emit('    }', level)
                    self.emit('    self().write_string(a.first);', level)
                    self.emit('    this->visit_symbol(*a.second);', level)
                    self.emit('}', level)
                    self.emit('for (auto &a : x.m_%s->get_scope()) {' % field.name, level)
                    self.emit('    if (ASR::is_a<ASR::Function_t>(*a.second)) {', level)
                    self.emit('        self().write_string(a.first);', level)
                    self.emit('        this->visit_symbol(*a.second);', level)
                    self.emit('    }', level)
                    self.emit('}', level)
            elif field.type == "string" and not field.seq:
                if field.opt:
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    'self().write_bool(true);', 3)
                    self.emit(    'self().write_string(x.m_%s);' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    'self().write_bool(false);', 3)
                    self.emit("}", 2)
                else:
                    self.emit('self().write_string(x.m_%s);' % field.name, 2)
            elif field.type == "int" and not field.seq:
                if field.opt:
                    self.emit("if (x.m_%s) {" % field.name, 2)
                    self.emit(    'self().write_bool(true);', 3)
                    self.emit(    'self().write_int64(x.m_%s);' % field.name, 3)
                    self.emit("} else {", 2)
                    self.emit(    'self().write_bool(false);', 3)
                    self.emit("}", 2)
                else:
                    self.emit('self().write_int64(x.m_%s);' % field.name, 2)
            elif field.type == "bool" and not field.seq and not field.opt:
                self.emit("if (x.m_%s) {" % field.name, 2)
                self.emit(    'self().write_bool(true);', 3)
                self.emit("} else {", 2)
                self.emit(    'self().write_bool(false);', 3)
                self.emit("}", 2)
            elif field.type == "float" and not field.seq and not field.opt:
                self.emit('self().write_float64(x.m_%s);' % field.name, 2)
            elif field.type in self.data.simple_types:
                if field.opt:
                    raise Exception("Unimplemented opt for field type: " + field.type);
                else:
                    self.emit('visit_%sType(x.m_%s);' \
                            % (field.type, field.name), 2)
            else:
                raise Exception("Unimplemented field type: " + field.type);

class DeserializationVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Deserialization Visitor base class")
        self.emit("")
        self.emit("template <class Struct>")
        self.emit("class DeserializationBaseVisitor : public BaseVisitor<Struct>")
        self.emit("{")
        self.emit("private:")
        self.emit(  "Struct& self() { return static_cast<Struct&>(*this); }", 1)
        self.emit("public:")
        self.emit(  "Allocator &al;", 1)
        self.emit(  "bool load_symtab_id;", 1)
        self.emit(  "std::map<uint64_t,SymbolTable*> id_symtab_map;", 1)
        self.emit(  r"DeserializationBaseVisitor(Allocator &al, bool load_symtab_id) : al{al}, load_symtab_id{load_symtab_id} {}", 1)
        self.emit_deserialize_node();
        self.mod = mod
        super(DeserializationVisitorVisitor, self).visitModule(mod)
        self.emit("};")

    def visitType(self, tp):
        super(DeserializationVisitorVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        assert isinstance(sum, asdl.Sum)
        if is_simple_sum(sum):
            self.emit("%sType deserialize_%s() {" % (args[0], args[0]), 1)
            self.emit(  'uint8_t t = self().read_int8();', 2)
            self.emit(  '%sType ty = static_cast<%sType>(t);' % (args[0], args[0]), 2)
            self.emit(  'return ty;', 2)
            self.emit("}", 1)
        else:
            for tp in sum.types:
                self.visit(tp, *args)
            self.emit("%s_t* deserialize_%s() {" % (subs["mod"], args[0]), 1)
            self.emit(  'uint8_t t = self().read_int8();', 2)
            self.emit(  '%s::%sType ty = static_cast<%s::%sType>(t);' % (subs["MOD"], args[0],
                subs["MOD"], args[0]), 2)
            self.emit(  'switch (ty) {', 2)
            for tp in sum.types:
                self.emit(    'case (%s::%sType::%s) : return self().deserialize_%s();' \
                    % (subs["MOD"], args[0], tp.name, tp.name), 3)
            self.emit(    'default : throw LCompilersException("Unknown type in deserialize_%s()");' % args[0], 3)
            self.emit(  '}', 2)
            self.emit(  'throw LCompilersException("Switch statement above was not exhaustive.");', 2)

            self.emit("}", 1)

    def emit_deserialize_node(self):
        name = "node"
        self.emit("%s_t* deserialize_%s() {" % (subs["mod"], name), 1)
        self.emit(  'uint8_t t = self().read_int8();', 2)
        self.emit(  '%s::%sType ty = static_cast<%s::%sType>(t);' % (subs["MOD"], subs["mod"],
            subs["MOD"], subs["mod"]), 2)
        self.emit(  'switch (ty) {', 2)
        for tp in sums:
            self.emit(    'case (%s::%sType::%s) : return self().deserialize_%s();' \
                % (subs["MOD"], subs["mod"], tp, tp), 3)
        self.emit(    'default : throw LCompilersException("Unknown type in deserialize_%s()");' % name, 3)
        self.emit(  '}', 2)
        self.emit(  'throw LCompilersException("Switch statement above was not exhaustive.");', 2)
        self.emit(  '}', 1)

    def visitProduct(self, prod, name):
        self.emit("%s_t deserialize_%s() {" % (name, name), 1)
        self.emit(  '%s_t x;' % (name), 2)
        for field in prod.fields:
            if field.seq:
                assert not field.opt
                assert field.type not in simple_sums
                if field.type in asdl.builtin_types:
                    if field.type == "identifier":
                        self.emit('{', 2)
                        self.emit('uint64_t n = self().read_int64();', 3)
                        self.emit("Vec<char*> v;", 3)
                        self.emit("v.reserve(al, n);", 3)
                        self.emit("for (uint64_t i=0; i<n; i++) {", 3)
                        self.emit("v.push_back(al, self().read_cstring());", 4)
                        self.emit('}', 3)
                        self.emit('x.m_%s = v.p;' % (field.name), 3)
                        self.emit('x.n_%s = v.n;' % (field.name), 3)
                        self.emit('}', 2)
                    else:
                        assert False
                else:
                    self.emit('{', 2)
                    self.emit('uint64_t n = self().read_int64();', 3)
                    if field.type in products:
                        self.emit("Vec<%s_t> v;" % (field.type), 3)
                    else:
                        self.emit("Vec<%s_t*> v;" % (field.type), 3)
                    self.emit("v.reserve(al, n);", 3)
                    self.emit("for (uint64_t i=0; i<n; i++) {", 3)
                    if field.type in products:
                        self.emit("v.push_back(al, self().deserialize_%s());" \
                             % (field.type), 4)
                    else:
                        self.emit("v.push_back(al, down_cast<%s_t>(self().deserialize_%s()));" % (field.type, field.type), 4)
                    self.emit('}', 3)
                    self.emit('x.m_%s = v.p;' % (field.name), 3)
                    self.emit('x.n_%s = v.n;' % (field.name), 3)
                    self.emit('}', 2)
            else:
                self.emit('{', 2)
                if field.opt:
                    self.emit("bool present=self().read_bool();", 3)
                if field.type in asdl.builtin_types:
                    if field.type == "identifier":
                        rhs = "self().read_cstring()"
                    elif field.type == "string":
                        rhs = "self().read_cstring()"
                    elif field.type == "int":
                        rhs = "self().read_int64()"
                    else:
                        print(field.type)
                        assert False
                elif field.type in simple_sums:
                    rhs = "deserialize_%s()" % (field.type)
                else:
                    assert field.type not in products
                    if field.type == "symbol":
                        rhs = "self().read_symbol()"
                    else:
                        rhs = "down_cast<%s_t>(deserialize_%s())" % (field.type,
                            field.type)
                if field.opt:
                    self.emit('if (present) {', 3)
                self.emit('x.m_%s = %s;' % (field.name, rhs), 4)
                if field.opt:
                    self.emit('} else {', 3)
                    self.emit(  'x.m_%s = nullptr;' % (field.name), 4)
                    self.emit('}', 3)
                self.emit('}', 2)
        self.emit(  'return x;', 2)
        self.emit("}", 1)

    def visitConstructor(self, cons, _):
        name = cons.name
        self.emit("%s_t* deserialize_%s() {" % (subs["mod"], name), 1)
        lines = []
        args = ["al", "loc"]
        for f in cons.fields:
            #type_ = convert_type(f.type, f.seq, f.opt, self.mod.name.lower())
            if f.seq:
                seq = "size_t n_%s; // Sequence" % f.name
                self.emit("%s" % seq, 2)
            else:
                seq = ""
            if f.seq:
                assert f.type not in self.data.simple_types
                if f.type not in asdl.builtin_types:
                    lines.append("n_%s = self().read_int64();" % (f.name))
                    if f.type in products:
                        lines.append("Vec<%s_t> v_%s;" % (f.type, f.name))
                    else:
                        lines.append("Vec<%s_t*> v_%s;" % (f.type, f.name))
                    lines.append("v_%s.reserve(al, n_%s);" % (f.name, f.name))
                    lines.append("for (size_t i=0; i<n_%s; i++) {" % (f.name))
                    if f.type in products:
                        lines.append("    v_%s.push_back(al, self().deserialize_%s());" % (f.name, f.type))
                    else:
                        if f.type == "symbol":
                            lines.append("    v_%s.push_back(al, self().read_symbol());" % (f.name))
                        else:
                            lines.append("    v_%s.push_back(al, %s::down_cast<%s::%s_t>(self().deserialize_%s()));" % (f.name,
                                subs["MOD"], subs["MOD"], f.type, f.type))
                    lines.append("}")
                else:
                    if f.type == "node":
                        lines.append("n_%s = self().read_int64();" % (f.name))
                        lines.append("Vec<%s_t*> v_%s;" % (subs["mod"], f.name))
                        lines.append("v_%s.reserve(al, n_%s);" % (f.name, f.name))
                        lines.append("for (size_t i=0; i<n_%s; i++) {" % (f.name))
                        lines.append("    v_%s.push_back(al, self().deserialize_node());" % (f.name))
                        lines.append("}")
                    elif f.type == "identifier":
                        lines.append("n_%s = self().read_int64();" % (f.name))
                        lines.append("Vec<char*> v_%s;" % (f.name))
                        lines.append("v_%s.reserve(al, n_%s);" % (f.name, f.name))
                        lines.append("for (size_t i=0; i<n_%s; i++) {" % (f.name))
                        lines.append("    v_%s.push_back(al, self().read_cstring());" % (f.name))
                        lines.append("}")
                    else:
                        print(f.type)
                        assert False
                args.append("v_%s.p" % (f.name))
                args.append("v_%s.n" % (f.name))
            else:
                # if builtin or simple types, handle appropriately
                if f.type in asdl.builtin_types:
                    if f.type == "identifier":
                        lines.append("char *m_%s;" % (f.name))
                        if f.opt:
                            lines.append("bool m_%s_present = self().read_bool();" \
                                % f.name)
                            lines.append("if (m_%s_present) {" % f.name)
                        lines.append("m_%s = self().read_cstring();" % (f.name))
                        if f.opt:
                            lines.append("} else {")
                            lines.append("m_%s = nullptr;" % (f.name))
                            lines.append("}")
                        args.append("m_%s" % (f.name))
                    elif f.type == "string":
                        lines.append("char *m_%s;" % (f.name))
                        if f.opt:
                            lines.append("bool m_%s_present = self().read_bool();" \
                                % f.name)
                            lines.append("if (m_%s_present) {" % f.name)
                        lines.append("m_%s = self().read_cstring();" % (f.name))
                        if f.opt:
                            lines.append("} else {")
                            lines.append("m_%s = nullptr;" % (f.name))
                            lines.append("}")
                        args.append("m_%s" % (f.name))
                    elif f.type == "int":
                        assert not f.opt
                        lines.append("int64_t m_%s = self().read_int64();" % (f.name))
                        args.append("m_%s" % (f.name))
                    elif f.type == "float":
                        assert not f.opt
                        lines.append("double m_%s = self().read_float64();" % (f.name))
                        args.append("m_%s" % (f.name))
                    elif f.type == "bool":
                        assert not f.opt
                        lines.append("bool m_%s = self().read_bool();" % (f.name))
                        args.append("m_%s" % (f.name))
                    elif f.type == "symbol_table":
                        assert not f.opt
                        # TODO: read the symbol table:
                        args.append("m_%s" % (f.name))
                        lines.append("uint64_t m_%s_counter = self().read_int64();" % (f.name))
                        if f.name == "parent_symtab":
                            # If this fails, it means we are referencing a
                            # symbol table that was not constructed yet. If
                            # that ever happens, we would have to remember
                            # this and come back later to fix the pointer.
                            lines.append("LCOMPILERS_ASSERT(id_symtab_map.find(m_%s_counter) != id_symtab_map.end());" % f.name)
                            lines.append("SymbolTable *m_%s = id_symtab_map[m_%s_counter];" % (f.name, f.name))
                        else:
                            # We construct a new table. It should not
                            # be present:
                            lines.append("LCOMPILERS_ASSERT(id_symtab_map.find(m_%s_counter) == id_symtab_map.end());" % f.name)
                            lines.append("SymbolTable *m_%s = al.make_new<SymbolTable>(nullptr);" % (f.name))
                            lines.append("if (load_symtab_id) m_%s->counter = m_%s_counter;" % (f.name, f.name))
                            lines.append("id_symtab_map[m_%s_counter] = m_%s;" % (f.name, f.name))
                            lines.append("{")
                            lines.append("    size_t n = self().read_int64();")
                            lines.append("    for (size_t i=0; i<n; i++) {")
                            lines.append("        std::string name = self().read_string();")
                            lines.append("        symbol_t *sym = down_cast<symbol_t>(deserialize_symbol());")
                            lines.append("        self().symtab_insert_symbol(*m_%s, name, sym);" % f.name)
                            lines.append("    }")
                            lines.append("}")
                    else:
                        print(f.type)
                        assert False
                else:
                    if f.type in products:
                        assert not f.opt
                        lines.append("%s::%s_t m_%s = self().deserialize_%s();" % (subs["MOD"], f.type, f.name, f.type))
                    else:
                        if f.type in simple_sums:
                            assert not f.opt
                            lines.append("%s::%sType m_%s = self().deserialize_%s();" % (subs["MOD"],
                                f.type, f.name, f.type))
                        else:
                            lines.append("%s::%s_t *m_%s;" % (subs["MOD"],
                                f.type, f.name))
                            if f.opt:
                                lines.append("if (self().read_bool()) {")
                            if f.type == "symbol":
                                if name == "ExternalSymbol":
                                    lines.append("// We skip the symbol for ExternalSymbol")
                                    lines.append("m_%s = nullptr;" % (f.name))
                                else:
                                    lines.append("m_%s = self().read_symbol();" % (f.name))
                            else:
                                lines.append("m_%s = %s::down_cast<%s::%s_t>(self().deserialize_%s());" % (
                                    f.name, subs["MOD"], subs["MOD"], f.type, f.type))
                            if f.opt:
                                lines.append("} else {")
                                lines.append("m_%s = nullptr;" % f.name)
                                lines.append("}")
                    args.append("m_%s" % (f.name))

        self.emit(    'Location loc;', 2)
        self.emit(    'loc.first = self().read_int64();', 2)
        self.emit(    'loc.last = self().read_int64();', 2)
        if subs["lcompiler"] == "lfortran":
            # Set the location to 0 for now, since we do not yet
            # support multiple files
            self.emit(    'loc.first = 0;', 2)
            self.emit(    'loc.last = 0;', 2)
        for line in lines:
            self.emit(line, 2)
        self.emit(    'return %s::make_%s_t(%s);' % (subs["MOD"], name, ", ".join(args)), 2)
        self.emit("}", 1)

class ExprTypeVisitor(ASDLVisitor):

    def __init__(self, stream, data):
        self.replace_expr = []
        self.is_expr = False
        self.is_product = False
        super(ExprTypeVisitor, self).__init__(stream, data)

    def emit(self, line, level=0, new_line=True):
        indent = "    "*level
        self.stream.write(indent + line)
        if new_line:
            self.stream.write("\n")

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Expression Type (`expr_type`) visitor")
        self.emit("""\
static inline ASR::ttype_t* expr_type0(const ASR::expr_t *f)
{
    LCOMPILERS_ASSERT(f != nullptr);
    switch (f->type) {""")

        super(ExprTypeVisitor, self).visitModule(mod)

        self.emit("""        default : throw LCompilersException("Not implemented");
    }
}
""")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(ExprTypeVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        self.is_expr = args[0] == 'expr'
        if self.is_expr:
            for tp in sum.types:
                self.visit(tp, *args)

    def visitProduct(self, prod, name):
        pass

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        if name == "Var":
            self.emit("""case ASR::exprType::%s: {
            ASR::symbol_t *s = ((ASR::%s_t*)f)->m_v;
            if (s->type == ASR::symbolType::ExternalSymbol) {
                ASR::ExternalSymbol_t *e = ASR::down_cast<ASR::ExternalSymbol_t>(s);
                LCOMPILERS_ASSERT(e->m_external);
                LCOMPILERS_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*e->m_external));
                s = e->m_external;
            } else if (s->type == ASR::symbolType::Function) {
                return ASR::down_cast<ASR::Function_t>(s)->m_function_signature;
            }
            return ASR::down_cast<ASR::Variable_t>(s)->m_type;
        }""" \
                    % (name, name), 2, new_line=False)
        elif name == "OverloadedBinOp":
            self.emit("case ASR::exprType::%s: { return expr_type0(((ASR::%s_t*)f)->m_overloaded); }"\
                    % (name, name), 2, new_line=False)
        else:
            self.emit("case ASR::exprType::%s: { return ((ASR::%s_t*)f)->m_type; }"\
                    % (name, name), 2, new_line=False)
        self.emit("")

    def visitField(self, field):
        pass

class ExprValueVisitor(ASDLVisitor):

    def __init__(self, stream, data):
        self.replace_expr = []
        self.is_expr = False
        self.is_product = False
        super(ExprValueVisitor, self).__init__(stream, data)

    def emit(self, line, level=0, new_line=True):
        indent = "    "*level
        self.stream.write(indent + line)
        if new_line:
            self.stream.write("\n")

    def visitModule(self, mod):
        self.emit("/" + "*"*78 + "/")
        self.emit("// Expression Value (`expr_value`) visitor")
        self.emit("""\
static inline ASR::expr_t* expr_value0(ASR::expr_t *f)
{
    LCOMPILERS_ASSERT(f != nullptr);
    switch (f->type) {""")

        super(ExprValueVisitor, self).visitModule(mod)

        self.emit("""        default : throw LCompilersException("Not implemented");
    }
}
""")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(ExprValueVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        self.is_expr = args[0] == 'expr'
        if self.is_expr:
            for tp in sum.types:
                self.visit(tp, *args)

    def visitProduct(self, prod, name):
        pass

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        if name == "Var":
            self.emit("""case ASR::exprType::%s: {
            ASR::symbol_t *s = ((ASR::%s_t*)f)->m_v;
            if (s->type == ASR::symbolType::ExternalSymbol) {
                ASR::ExternalSymbol_t *e = ASR::down_cast<ASR::ExternalSymbol_t>(s);
                LCOMPILERS_ASSERT(!ASR::is_a<ASR::ExternalSymbol_t>(*e->m_external));
                s = e->m_external;
            }
            return ASR::down_cast<ASR::Variable_t>(s)->m_value;
        }""" \
                    % (name, name), 2, new_line=False)
        elif name.endswith("Constant") or name == "IntegerBOZ":
            self.emit("case ASR::exprType::%s: { return f; }"\
                    % (name), 2, new_line=False)
        else:
            self.emit("case ASR::exprType::%s: { return ((ASR::%s_t*)f)->m_value; }"\
                    % (name, name), 2, new_line=False)
        self.emit("")

    def visitField(self, field):
        pass

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


HEAD = r"""#ifndef LFORTRAN_%(MOD2)s_H
#define LFORTRAN_%(MOD2)s_H

// Generated by grammar/asdl_cpp.py

#include <libasr/alloc.h>
#include <libasr/location.h>
#include <libasr/colors.h>
#include <libasr/containers.h>
#include <libasr/exception.h>
#include <libasr/asr_scopes.h>


namespace LCompilers::%(MOD)s {

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
    LCOMPILERS_ASSERT(f != nullptr);
    LCOMPILERS_ASSERT(is_a<T>(*f));
    return (T*)f;
}

// Cast two levels down

template <class T>
static inline T* down_cast2(const %(mod)s_t *f)
{
    typedef typename T::parent_type ptype;
    ptype *t = down_cast<ptype>(f);
    return down_cast<T>(t);
}

"""

FOOT = r"""} // namespace LCompilers::%(MOD)s

#endif // LFORTRAN_%(MOD2)s_H
"""

visitors = [ASTNodeVisitor0, ASTNodeVisitor1, ASTNodeVisitor,
        ASTVisitorVisitor1, ASTVisitorVisitor1b, ASTVisitorVisitor2,
        ASTWalkVisitorVisitor, TreeVisitorVisitor, PickleVisitorVisitor,
        JsonVisitorVisitor, SerializationVisitorVisitor, DeserializationVisitorVisitor]


def main(argv):
    if len(argv) == 3:
        def_file, out_file = argv[1:]
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
        "MOD2": mod.name.upper(),
        "mod": mod.name.lower(),
        "types": types_,
    }
    if subs["MOD"] == "LPYTHON":
        subs["MOD"] = "LPython::AST"
        subs["mod"] = "ast"
        subs["lcompiler"] = "lpython"
    elif subs["MOD"] == "AST":
         subs["MOD"] = "LFortran::AST"
         subs["lcompiler"] = "lfortran"
    else:
        subs["lcompiler"] = "lfortran"
    is_asr = (mod.name.upper() == "ASR")
    fp = open(out_file, "w", encoding="utf-8")
    try:
        fp.write(HEAD % subs)
        for visitor in visitors:
            visitor(fp, data).visit(mod)
            fp.write("\n\n")
        if not is_asr:
            fp.write(FOOT % subs)
    finally:
        if not is_asr:
            fp.close()

    try:
        if is_asr:
            ExprStmtDuplicatorVisitor(fp, data).visit(mod)
            fp.write("\n\n")
            ExprBaseReplacerVisitor(fp, data).visit(mod)
            fp.write("\n\n")
            StmtBaseReplacerVisitor(fp, data).visit(mod)
            fp.write("\n\n")
            CallReplacerOnExpressionsVisitor(fp, data).visit(mod)
            fp.write("\n\n")
            ExprTypeVisitor(fp, data).visit(mod)
            fp.write("\n\n")
            ExprValueVisitor(fp, data).visit(mod)
            fp.write("\n\n")
            fp.write(FOOT % subs)
    finally:
        fp.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv))
