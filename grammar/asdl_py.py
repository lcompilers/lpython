"""
Generate AST node definitions from an ASDL description.
"""

import sys
import os

sys.path.append("lfortran/src/libasr")
import asdl

products = []
sums = []

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


class ASTNodeVisitor(ASDLVisitor):

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitSum(self, sum, base):
        if is_simple_sum(sum):
            self.emit("class %s(AST): # Sum" % (base,))
            self.emit(    "pass", 1)
            self.emit("")
            self.emit("")
            for i, cons in enumerate(sum.types):
                self.emit("class %s(%s): # Type" % (cons.name, base))
                self.emit(    "ntype = %d # Corresponds to Enum in C++" % i, 1)
                self.emit(    "pass", 1)
                self.emit("")
                self.emit("")
        else:
            self.emit("class %s(AST): # Sum" % (base,))
            if sum.attributes:
                self.emit("")
                self.emit("def __init__(self, %s):" \
                    % attr_to_args(sum.attributes), 1)
                for attr in sum.attributes:
                    self.visit(attr)
                self.emit(    "self._type = None", 2)
            else:
                self.emit("pass", 1)
            self.emit("")
            self.emit("")
            for i, cons in enumerate(sum.types):
                self.cons_i = i
                self.visit(cons, base, sum.attributes)
                self.emit("")

    def visitProduct(self, product, name):
        self.emit("class %s(AST): # Product" % (name,))
        self.emit("")
        self._fields = []
        self.make_constructor(product.fields, product)
        s = ", ".join(self._fields)
        if len(self._fields) == 1: s = s + ","
        self.emit("self._fields = (%s)" % s, 2)
        self.emit("self._type = None", 2)
        self.emit("")
        self.emit("def walkabout(self, visitor):", 1)
        self.emit("return visitor.visit_%s(self)" % (name,), 2)
        self.emit("")
        self.emit("")

    def make_constructor(self, fields, node, extras=None, base=None):
        if fields or extras:
            arg_fields = fields + extras if extras else fields
            args = attr_to_args(arg_fields)
            self.emit("def __init__(self, %s):" % args, 1)
            for field in fields:
                self.visit(field)
            if extras:
                base_args = ", ".join(str(field.name) for field in extras)
                self.emit("%s.__init__(self, %s)" % (base, base_args), 2)
        else:
            self.emit("def __init__(self):", 1)

    def visitConstructor(self, cons, base, extra_attributes):
        self.emit("class %s(%s): # Constructor" % (cons.name, base))
        self.emit(    "ntype = %d # 88" % self.cons_i, 1)
        self.emit("")
        self._fields = []
        self.make_constructor(cons.fields, cons, extra_attributes, base)
        s = ", ".join(self._fields)
        if len(self._fields) == 1: s = s + ","
        self.emit("self._fields = (%s)" % s, 2)
        self.emit("self._type = None", 2)
        self.emit("")
        self.emit("def walkabout(self, visitor):", 1)
        self.emit("return visitor.visit_%s(self)" % (cons.name,), 2)
        self.emit("")
        self.emit("")

    def visitField(self, field):
        self.emit("self.%s = %s" % (field.name, field.name), 2)
        if field.seq:
            self.emit('assert isinstance(%s, list)' % field.name, 2)
            self.emit('for x in %s:' % field.name, 2)
            type_ = field.type
            if type_ == "string":
                type_ = "str"
            elif type_ == "identifier":
                type_ = "str"
            elif type_ == "constant":
                type_ = "bool"
            elif type_ == "node":
                type_ = "AST"
            self.emit('checkinstance(x, %s, %r)' % (type_, field.opt), 3)
        else:
            type_ = field.type
            if type_ == "string":
                type_ = "str"
            elif type_ == "identifier":
                type_ = "str"
            elif type_ == "constant":
                type_ = "bool"
            elif type_ == "node":
                type_ = "AST"
            elif type_ == "symbol_table":
                type_ = "object"
            self.emit('checkinstance(%s, %s, %r)' % \
                    (field.name, type_, field.opt), 2)
        self._fields.append("'" + field.name + "'")


class ASTVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("class ASTVisitor(object):")
        self.emit("")
        self.emit(    "def visit_sequence(self, seq):", 1)
        self.emit(        "if seq is not None:", 2)
        self.emit(        "for node in seq:", 3)
        self.emit(            "self.visit(node)", 4)
        self.emit("")
        self.emit(    "def visit(self, node):", 1)
        self.emit(        "return node.walkabout(self)", 2)
        self.emit("")
        self.emit(    "def default_visitor(self, node):", 1)
        self.emit(        "raise NodeVisitorNotImplemented", 2)
        self.emit("")
        super(ASTVisitorVisitor, self).visitModule(mod)
        self.emit("")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(ASTVisitorVisitor, self).visitType(tp, tp.name)

    def visitProduct(self, prod, name):
        self.emit(    "def visit_%s(self, node):" % (name,), 1)
        self.emit(        "return self.default_visitor(node)", 2)

    def visitConstructor(self, cons, _):
        self.emit(    "def visit_%s(self, node):" % (cons.name,), 1)
        self.emit(        "return self.default_visitor(node)", 2)


class GenericASTVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("class GenericASTVisitor(ASTVisitor):")
        self.emit("")
        super(GenericASTVisitorVisitor, self).visitModule(mod)
        self.emit("")

    def visitType(self, tp):
        if not (isinstance(tp.value, asdl.Sum) and
                is_simple_sum(tp.value)):
            super(GenericASTVisitorVisitor, self).visitType(tp, tp.name)

    def visitProduct(self, prod, name):
        self.make_visitor(name, prod.fields)

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields)

    def make_visitor(self, name, fields):
        self.emit("def visit_%s(self, node):" % (name,), 1)
        have_body = False
        for field in fields:
            if self.visitField(field):
                have_body = True
        if not have_body:
            self.emit("pass", 2)
        self.emit("")

    def visitField(self, field):
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            level = 2
            template = "self.visit(node.%s)"
            if field.seq:
                template = "self.visit_sequence(node.%s)"
            elif field.opt:
                self.emit("if node.%s:" % (field.name,), 2)
                level = 3
            self.emit(template % (field.name,), level)
            return True
        return False


class SerializationVisitorVisitor(ASDLVisitor):

    def visitModule(self, mod):
        self.emit("#" + "*"*79)
        self.emit("## Serialization Visitor base class")
        self.emit("")
        self.emit("class SerializationBaseVisitor(ASTVisitor):")
        self.mod = mod
        super(SerializationVisitorVisitor, self).visitModule(mod)

    def visitType(self, tp):
        super(SerializationVisitorVisitor, self).visitType(tp, tp.name)

    def visitSum(self, sum, *args):
        assert isinstance(sum, asdl.Sum)
        if is_simple_sum(sum):
            name = args[0]
            self.make_simple_sum_visitor(name, sum.types)
        else:
            for tp in sum.types:
                self.visit(tp, *args)

    def visitProduct(self, prod, name):
        self.make_visitor(name, prod.fields, False)

    def visitConstructor(self, cons, _):
        self.make_visitor(cons.name, cons.fields, True)

    def make_visitor(self, name, fields, cons):
        self.emit("def visit_%s(self, x: %s):" % (name, name), 1)
        if cons:
            self.emit(    'self.write_int8(x.ntype)', 2)
            self.emit(    'self.write_int64(x.first)', 2)
            self.emit(    'self.write_int64(x.last)', 2)
        self.used = False
        for n, field in enumerate(fields):
            self.visitField(field, cons, name)
        if not self.used:
            pass

    def make_simple_sum_visitor(self, name, types):
        self.emit("def visit_%s(self, x: %s):" % (name, name), 1)
        self.emit(    'self.write_int8(x.ntype)', 2)

    def visitField(self, field, cons, cons_name):
        if (field.type not in asdl.builtin_types and
            field.type not in self.data.simple_types):
            self.used = True
            level = 2
            if field.type in products:
                if field.opt:
                    template = "self.visit_%s(x.%s)" % (field.type, field.name)
                else:
                    template = "self.visit_%s(x.%s)" % (field.type, field.name)
            else:
                template = "self.visit(x.%s)" % (field.name)
            if field.seq:
                self.emit('self.write_int64(len(x.%s))' % field.name, level)
                self.emit("for i in range(len(x.%s)):" % field.name, level)
                if field.type in sums:
                    self.emit("self.visit(x.%s[i])" % (field.name), level+1)
                else:
                    self.emit("self.visit(x.%s[i])" % (field.name), level+1)
            elif field.opt:
                self.emit("if x.%s:" % field.name, 2)
                self.emit(    'self.write_bool(True)', 3)
                self.emit(template, 3)
                self.emit("else:", 2)
                self.emit(    'self.write_bool(False)', 3)
            else:
                self.emit(template, level)
        else:
            if field.type == "identifier":
                if field.seq:
                    assert not field.opt
                    level = 2
                    self.emit('self.write_int64(len(x.%s))' % field.name, level)
                    self.emit("for i in range(len(x.%s)):" % field.name, level)
                    self.emit("self.write_string(x.%s[i])" % (field.name), level+1)
                else:
                    if field.opt:
                        self.emit("if x.%s:" % field.name, 2)
                        self.emit(    'self.write_bool(True)', 3)
                        self.emit(    'self.write_string(x.%s)' % field.name, 3)
                        self.emit("else:", 2)
                        self.emit(    'self.write_bool(False)', 3)
                    else:
                        self.emit('self.write_string(x.%s)' % field.name, 2)
            elif field.type == "string" and not field.seq:
                if field.opt:
                    self.emit("if x.%s:" % field.name, 2)
                    self.emit(    'self.write_bool(True)', 3)
                    self.emit(    'self.write_string(x.%s)' % field.name, 3)
                    self.emit("else:", 2)
                    self.emit(    'self.write_bool(False)', 3)
                else:
                    self.emit('self.write_string(x.%s);' % field.name, 2)
            elif field.type == "int" and not field.seq:
                if field.opt:
                    self.emit("if x.%s:" % field.name, 2)
                    self.emit(    'self.write_bool(True)', 3)
                    self.emit(    'self.write_int64(x.%s)' % field.name, 3)
                    self.emit("else:", 2)
                    self.emit(    'self.write_bool(False)', 3)
                else:
                    self.emit('self.write_int64(x.%s)' % field.name, 2)
            elif field.type == "bool" and not field.seq and not field.opt:
                self.emit("if x.%s:" % field.name, 2)
                self.emit(    'self.write_bool(True)', 3)
                self.emit("else:", 2)
                self.emit(    'self.write_bool(False)', 3)
            elif field.type == "float" and not field.seq and not field.opt:
                self.emit('self.write_float64(x.%s)' % field.name, 2)
            elif field.type in self.data.simple_types:
                if field.opt:
                    raise Exception("Unimplemented opt for field type: " + field.type);
                else:
                    self.emit('self.visit_%s(x.%s)' \
                            % (field.type, field.name), 2)
            else:
                raise Exception("Unimplemented field type: " + field.type);

class CollectVisitor(ASDLVisitor):

    def visitType(self, tp):
        self.visit(tp.value, tp.name)

    def visitSum(self, sum, base):
        if not is_simple_sum(sum):
            sums.append(base);

    def visitProduct(self, product, name):
        products.append(name)



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


HEAD = r"""# Generated by utils/asdl_py.py

class AST(object):
    _attrs_ = ['lineno', 'col_offset']

    def __init__(self):
        self._fields = ()
        self._type = None

    def walkabout(self, visitor):
        raise AssertionError("walkabout() implementation not provided")

    def mutate_over(self, visitor):
        raise AssertionError("mutate_over() implementation not provided")


class NodeVisitorNotImplemented(Exception):
    pass

def checkinstance(a, b, opt=False):
    if opt and a is None:
        return
    if not isinstance(a, b):
        if isinstance(a, AST):
            a_dump = a
        else:
            a_dump = a
        print("Wrong instance: %s, types: a=%s; b=%s" % (a_dump, type(a), b))
    assert isinstance(a, b)

"""

visitors = [CollectVisitor, ASTNodeVisitor, ASTVisitorVisitor,
    GenericASTVisitorVisitor, SerializationVisitorVisitor]


def main(argv):
    if len(argv) == 4:
        def_file, out_file, util_import_part = argv[1:]
    elif len(argv) == 1:
        print("Assuming default values of Python.asdl and python_ast.py")
        here = os.path.dirname(__file__)
        def_file = os.path.join(here, "Python.asdl")
        out_file = os.path.join(here, "..", "src", "runtime", "python_ast.py")
        util_import_part = ".utils"
    else:
        print("invalid arguments")
        return 2
    mod = asdl.parse(def_file)
    data = ASDLData(mod)
    fp = open(out_file, "w")
    try:
        fp.write(HEAD.format(util_import_part))
        for visitor in visitors:
            visitor(fp, data).visit(mod)
    finally:
        fp.close()


if __name__ == "__main__":
    sys.exit(main(sys.argv))
