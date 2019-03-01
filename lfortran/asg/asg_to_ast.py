from ..ast import ast
from . import asg

class ASG2ASTVisitor(asg.ASTVisitor):

    def visit_sequence(self, seq):
        r = []
        if seq is not None:
            for node in seq:
                r.append(self.visit(node))
        return r

    def visit_Module(self, node):
        decl = []
        contains = []
        for s in node.symtab.symbols:
            sym = node.symtab.symbols[s]
            if isinstance(sym, asg.Function):
                if sym.body:
                    contains.append(self.visit(sym))
                else:
                    decl.append(
                        ast.Interface2(procs=[self.visit(sym)])
                    )
            else:
                raise NotImplementedError()
        return ast.Module(name=node.name, decl=decl, contains=contains)

    def visit_Assignment(self, node):
        target = self.visit(node.target)
        value = self.visit(node.value)
        return ast.Assignment(target, value)

    def visit_BinOp(self, node):
        left = self.visit(node.left)
        right = self.visit(node.right)
        if isinstance(node.op, asg.Add):
            op = ast.Add()
        elif isinstance(node.op, asg.Mul):
            op = ast.Mul()
        else:
            raise NotImplementedError()
        return ast.BinOp(left, op, right)

    def visit_Variable(self, node):
        return ast.Name(id=node.name)

    def visit_Num(self, node):
        return ast.Num(n=node.n)

    def visit_Integer(self, node):
        if node.kind == 4:
            return "integer"
        else:
            return "integer(kind=%d)" % node.kind

    def visit_Function(self, node):
        body = self.visit_sequence(node.body)
        args = []
        decl = []
        for arg in node.args:
            args.append(ast.arg(arg=arg.name))
            stype = self.visit(arg.type)
            attrs = []
            if arg.intent:
                attrs = [
                    ast.Attribute(name="intent",
                    args=[ast.attribute_arg(arg=arg.intent)]),
                ]
            decl.append(ast.Declaration(vars=[
                ast.decl(sym=arg.name, sym_type=stype,
                attrs=attrs)]))
        for s in node.symtab.symbols:
            sym = node.symtab.symbols[s]
            if sym.dummy:
                continue
            stype = self.visit(sym.type)
            decl.append(ast.Declaration(vars=[
                ast.decl(sym=sym.name, sym_type=stype)]))
        return_type = self.visit(node.return_var.type)
        return_var = self.visit(node.return_var)
        return ast.Function(
            name=node.name, args=args, return_type=return_type,
                return_var=return_var,
            decl=decl, body=body)


def asg_to_ast(a):
    v = ASG2ASTVisitor()
    return v.visit(a)