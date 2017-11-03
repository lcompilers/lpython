import antlr4
from antlr4.error.ErrorListener import ErrorListener
from .fortranLexer import fortranLexer
from .fortranParser import fortranParser
from .fortranVisitor import fortranVisitor
from .ast import ast

class SyntaxErrorException(Exception):
    pass

def get_line(s, l):
    return s.split("\n")[l-1]

class VerboseErrorListener(ErrorListener):
    def syntaxError(self, recognizer, offendingSymbol, line, column, msg, e):
        stack = recognizer.getRuleInvocationStack()
        stack.reverse()
        print("rule stack: ", str(stack))
        print("%d:%d: %s" % (line, column, msg))
        print(get_line(self.source, line))
        print(" "*(column-1) + "^")
        raise SyntaxErrorException("Syntax error.")

class ASTBuilderVisitor(fortranVisitor):
    """
    Converts the ANTLR parse tree to AST.

    The file fortran.g4 defines the parse tree nodes. The file Fortran.asdl
    defines the AST nodes. This visitor converts from the first to the second.
    When fortran.g4 is changed, then this visitor must be changed accordingly
    because the parse tree nodes change. When Fortran.asdl is changed, then
    this visitor also must be updated, because the AST nodes change.

    The AST that this visitor returns does not depend on ANTLR in any way.
    ANTLR is just used to construct it.
    """

    def __init__(self):
        pass

    def aggregateResult(self, aggregate, nextResult):
        if aggregate is None:
            return nextResult
        elif nextResult is None:
            return aggregate
        else:
            if isinstance(aggregate, list) and isinstance(nextResult, list):
                return aggregate + nextResult
            elif isinstance(aggregate, list):
                return aggregate + [nextResult]
            elif isinstance(nextResult, list):
                return [aggregate] + nextResult
            else:
                return [aggregate, nextResult]


    # Visit a parse tree produced by fortranParser#program.
    def visitProgram(self, ctx:fortranParser.ProgramContext):
        name = ctx.ID(0).getText()
        body = self.visit(ctx.sub_block())
        if not isinstance(body, list):
            body = [body]
        return ast.Program(name, body)

    # Visit a parse tree produced by fortranParser#assignment_statement.
    def visitAssignment_statement(self, ctx:fortranParser.Assignment_statementContext):
        if ctx.op.text == "=>":
            raise Exception("operator => not implemented")
        assert ctx.op.text == "="
        lhs = ctx.ID().getText()
        value = self.visit(ctx.expr())
        return ast.Assignment(lhs, value, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#var_decl.
    def visitVar_decl(self, ctx:fortranParser.Var_declContext):
        d = []
        for v in ctx.var_sym_decl():
            sym = v.ID().getText()
            sym_type = ctx.var_type().getText()
            d.append(ast.decl(sym, sym_type))
        return ast.Declaration(d, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#print_statement.
    def visitPrint_statement(self, ctx:fortranParser.Print_statementContext):
        values = []
        for expr in ctx.expr_list().expr():
            v = self.visit(expr)
            values.append(v)
        return ast.Print(fmt=None, values=values, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_string.
    def visitExpr_string(self, ctx:fortranParser.Expr_stringContext):
        s = ctx.STRING().getText()
        if s[0] == '"':
            s = s[1:-1]
            s = s.replace('""', '"')
        elif s[0] == "'":
            s = s[1:-1]
            s = s.replace("''", "'")
        else:
            raise Exception("Incorrect string")
        return ast.Str(s=s, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#number_real.
    def visitNumber_real(self, ctx:fortranParser.Number_realContext):
        num = ctx.NUMBER().getText()
        return ast.Num(n=num, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_id.
    def visitExpr_id(self, ctx:fortranParser.Expr_idContext):
        v = ctx.ID().getText()
        return ast.Name(id=v, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_muldiv.
    def visitExpr_addsub(self, ctx:fortranParser.Expr_muldivContext):
        op = ctx.op.text
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        if op == '+':
            return ast.BinOp(left=lhs, op=ast.Add(), right=rhs,
                    lineno=1, col_offset=1)
        else:
            return ast.BinOp(left=lhs, op=ast.Sub(), right=rhs,
                    lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_muldiv.
    def visitExpr_muldiv(self, ctx:fortranParser.Expr_muldivContext):
        op = ctx.op.text
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        if op == '*':
            return ast.BinOp(left=lhs, op=ast.Mul(), right=rhs,
                    lineno=1, col_offset=1)
        else:
            return ast.BinOp(left=lhs, op=ast.Div(), right=rhs,
                    lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_pow.
    def visitExpr_pow(self, ctx:fortranParser.Expr_powContext):
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        return ast.BinOp(left=lhs, op=ast.Pow(), right=rhs,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_truefalse.
    def visitExpr_truefalse(self, ctx:fortranParser.Expr_truefalseContext):
        op = ctx.op.text
        if op == '.true.':
            return ast.Constant(value=True, lineno=1, col_offset=1)
        else:
            return ast.Constant(value=False, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_unary.
    def visitExpr_unary(self, ctx:fortranParser.Expr_unaryContext):
        op = ctx.op.text
        rhs = self.visit(ctx.expr())
        if op == '+':
            return ast.UnaryOp(op=ast.UAdd(), operand=rhs,
                    lineno=1, col_offset=1)
        else:
            return ast.UnaryOp(op=ast.USub(), operand=rhs,
                    lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_rel.
    def visitExpr_rel(self, ctx:fortranParser.Expr_relContext):
        op = ctx.op.text
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        ops = {
            "==": ast.Eq(),
            "/=": ast.NotEq(),
            "<": ast.Lt(),
            "<=": ast.LtE(),
            ">": ast.Gt(),
            ">=": ast.GtE(),
        }
        return ast.Compare(left=lhs, op=ops[op], right=rhs,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_nest.
    def visitExpr_nest(self, ctx:fortranParser.Expr_nestContext):
        return self.visit(ctx.expr())

    # Visit a parse tree produced by fortranParser#stop_statement.
    def visitStop_statement(self, ctx:fortranParser.Stop_statementContext):
        if ctx.NUMBER():
            num = int(ctx.NUMBER().getText())
        else:
            num = None
        return ast.Stop(code=num, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#error_stop_statement.
    def visitError_stop_statement(self, ctx:fortranParser.Error_stop_statementContext):
        return ast.ErrorStop(lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#if_single_line.
    def visitIf_single_line(self, ctx:fortranParser.If_single_lineContext):
        cond = self.visit(ctx.if_cond().expr())
        body = self.visit(ctx.statement())
        if not isinstance(body, list):
            body = [body]
        return ast.If(test=cond, body=body, orelse=[], lineno=1, col_offset=1)


def parse(source):
    """
    Parse the `source` string into an AST node.

    We first call ANTLR4 to convert the `source` string into a parse tree. Then
    we convert the parse tree into an AST.
    """
    stream = antlr4.InputStream(source)
    lexer = fortranLexer(stream)
    tokens = antlr4.CommonTokenStream(lexer)
    parser = fortranParser(tokens)
    parser.removeErrorListeners()
    err = VerboseErrorListener()
    err.source = source
    parser.addErrorListener(err)
    parse_tree = parser.root()
    v = ASTBuilderVisitor()
    ast_tree = v.visit(parse_tree)
    return ast_tree

def parse_file(filename):
    source = open(filename).read()
    return parse(source)

def iter_fields(node):
    """
    Yield a tuple of ``(fieldname, value)`` for each field in ``node._fields``
    that is present on *node*.
    """
    for field in node._fields:
        try:
            yield field, getattr(node, field)
        except AttributeError:
            pass


def dump(node, annotate_fields=True, include_attributes=False):
    """
    Return a formatted dump of the tree in *node*.  This is mainly useful for
    debugging purposes.  The returned string will show the names and the values
    for fields.  This makes the code impossible to evaluate, so if evaluation is
    wanted *annotate_fields* must be set to False.  Attributes such as line
    numbers and column offsets are not dumped by default.  If this is wanted,
    *include_attributes* can be set to True.
    """
    def _format(node):
        if isinstance(node, ast.AST):
            fields = [(a, _format(b)) for a, b in iter_fields(node)]
            rv = '%s(%s' % (node.__class__.__name__, ', '.join(
                ('%s=%s' % field for field in fields)
                if annotate_fields else
                (b for a, b in fields)
            ))
            if include_attributes and node._attributes:
                rv += fields and ', ' or ' '
                rv += ', '.join('%s=%s' % (a, _format(getattr(node, a)))
                                for a in node._attributes)
            return rv + ')'
        elif isinstance(node, list):
            return '[%s]' % ', '.join(_format(x) for x in node)
        return repr(node)
    if not isinstance(node, ast.AST):
        raise TypeError('expected AST, got %r' % node.__class__.__name__)
    return _format(node)
