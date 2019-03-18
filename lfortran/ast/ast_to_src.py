from . import ast

class FortranPrinterVisitor(ast.ASTVisitor):

    def indent(self, lines, spaces=4):
        return [" "*spaces+x if x != "" else x for x in lines]

    def visit_sequence(self, seq, separate_line=False):
        lines = []
        if seq is not None:
            for node in seq:
                lines.extend(self.visit(node))
                if separate_line:
                    if node != seq[-1]:
                        lines.append("")
        return lines

    def visit_TranslationUnit(self, node):
        # TODO: Separate modules by 2 lines, functions by 1 line
        # and statements/expressions by 0 lines.
        return self.visit_sequence(node.items)

    def visit_Module(self, node):
        use = self.visit_sequence(node.use)
        decl = self.visit_sequence(node.decl, separate_line=True)
        contains = self.visit_sequence(node.contains, separate_line=True)
        lines = [
            "module %s" % node.name,
        ] + use + [
            "implicit none",
            "",
        ] + decl + [
            "",
            "contains",
            "",
        ] + contains + [
            "",
            "end module"
            "",
        ]
        return lines

    def visit_Interface2(self, node):
        ifaces = self.visit_sequence(node.procs, separate_line=True)
        lines = [
            "interface",
        ]
        lines.extend(self.indent(ifaces))
        lines.extend([
            "end interface",
        ])
        return lines

    def visit_Function(self, node):
        use = self.visit_sequence(node.use)
        if node.return_type:
            return_type = node.return_type + " "
        else:
            return_type = ""
        if node.return_var:
            return_var = " result(%s)" % self.visit(node.return_var)
        else:
            return_var = ""
        if node.bind:
            parts = []
            for k in node.bind.args:
                s = ""
                if k.arg:
                    s += k.arg + "="
                s += self.visit(k.value)
                parts.append(s)
            bind = " bind(" + ", ".join(parts) + ")"
        else:
            bind = ""
        lines = [
            return_type + "function %s(%s)" % (node.name,
            ", ".join([str(x.arg) for x in node.args])) + return_var + bind,
        ] + use \
            + self.visit_sequence(node.decl) \
            + self.visit_sequence(node.body) + [
            "end function"
        ]
        return lines

    def visit_Declaration(self, node):
        lines = []
        for decl in node.vars:
            parts = [decl.sym_type]
            for attr in decl.attrs:
                parts.append(self.visit(attr))
            dims = []
            for dim in decl.dims:
                # If a bound is None, we do not print it, otherwise we
                # explicitly print it (even if it is 1).
                lb, ub = dim.start, dim.end
                if lb:
                    lb = self.visit(lb)
                else:
                    lb = ""
                if ub:
                    ub = self.visit(ub)
                else:
                    ub = ""
                if ub == "":
                    # assumed-shape
                    dims.append("%s:%s" % (lb, ub))
                else:
                    # explicit-shape
                    if lb == "":
                        dims.append("%s" % (ub))
                    else:
                        dims.append("%s:%s" % (lb, ub))
            if dims:
                sdim = "("+",".join(dims)+")"
            else:
                sdim = ""

            lines.append(", ".join(parts) + " :: " + decl.sym + sdim)
        return lines

    def visit_Attribute(self, node):
        args = []
        for arg in node.args:
            args.append(arg.arg)
        s = node.name
        if args:
            s += "(" + ",".join(args) + ")"
        return s

    def visit_Assignment(self, node):
        target = self.visit(node.target)
        value = self.visit(node.value)
        return [target + " = " + value]

    def visit_Name(self, node):
        return node.id

    def visit_Num(self, node):
        return str(node.n)

    def visit_Str(self, node):
        return '"' + node.s + '"'

    def visit_BinOp(self, node):
        left = self.visit(node.left)
        right = self.visit(node.right)
        if isinstance(node.op, ast.Add):
            op = "+"
            return "%s %s %s" % (left, op, right)
        elif isinstance(node.op, ast.Mul):
            op = "*"
            return "(%s) %s (%s)" % (left, op, right)
        else:
            raise NotImplementedError()

    def visit_FuncCallOrArray(self, node):
        return node.func + "(" + \
            ", ".join([self.visit(arg) for arg in node.args]) + ")"

    def visit_FuncCall(self, node):
        return node.func + "(" + \
            ", ".join([self.visit(arg) for arg in node.args]) + ")"

    def visit_Use(self, node):
        if node.symbols:
            syms = ", only: " + ", ".join([self.visit(x) for x in node.symbols])
        else:
            syms = ""
        return ["use " + self.visit(node.module) + syms]

    def visit_use_symbol(self, node):
        if node.rename:
            s = "%s => %s" % (node.rename, node.sym)
        else:
            s = "%s" % node.sym
        return s


def ast_to_src(a):
    v = FortranPrinterVisitor()
    lines = v.visit(a)
    return "\n".join(lines) + "\n"
