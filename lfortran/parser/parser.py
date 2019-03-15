"""
# Fortran Parser

This module provides a parser implementation for the `ast` module. This module
should not be used directly, use `ast.src_to_ast()` instead.

Internally, `ast.src_to_ast()` calls `antlr_parse()` provided by this module.  The
`antlr_parse()` function accepts source code as a string and then uses ANTLR4
to create a parse tree and then converts the parse tree to an AST (using the
classes from the `ast` module) and returns it.

The returned AST (and thus the rest of the code) does not depend on ANTLR in
any way. As such, alternative parser implementations can be used --- all they
have to do is to return an AST, and one can just update the `ast.src_to_ast()` to
use them. The rest of the code does not depend on the parser implementation.

"""

import antlr4
from antlr4.error.ErrorListener import ErrorListener
from .fortranLexer import fortranLexer
from .fortranParser import fortranParser
from .fortranVisitor import fortranVisitor
from ..ast import ast, utils

def get_line(s, l):
    return s.split("\n")[l-1]

class VerboseErrorListener(ErrorListener):
    def syntaxError(self, recognizer, offendingSymbol, line, column, msg, e):
        stack = recognizer.getRuleInvocationStack()
        stack.reverse()
        # Here is how to get a rule stack: str(stack)
        message = "Syntax Error:%d:%d: %s\n%s\n%s\n" % (line, column, msg,
            get_line(self.source, line), " "*(column-1) + "^")
        raise utils.SyntaxErrorException(message)

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
        name = ctx.ident(0).getText()
        use, decl, body, contains = self.process_sub_block(ctx.sub_block())
        return ast.Program(name=name, use=use, decl=decl, body=body,
            contains=contains)

    def process_sub_block(self, ctx:fortranParser.Sub_blockContext):
        # Returns (use=[], decl=[], body=[], contains=[])
        use = []
        decl = []
        for u in ctx.use_statement():
            use.append(self.visit(u))
        for u in ctx.var_decl():
            decl.append(self.visit(u))
        if ctx.statements():
            body = self.statements2list(ctx.statements())
        else:
            body = []
        if ctx.contains_block():
            contains = self.contains_block2list(ctx.contains_block())
        else:
            contains = []
        return (use, decl, body, contains)

    # Visit a parse tree produced by fortranParser#module.
    def visitModule(self, ctx:fortranParser.ModuleContext):
        name = ctx.ident(0).getText()
        use = []
        decl = []
        for u in ctx.use_statement():
            use.append(self.visit(u))
        for u in ctx.module_decl():
            decl.append(self.visit(u))
        if ctx.contains_block():
            contains = self.contains_block2list(ctx.contains_block())
        else:
            contains = []
        return ast.Module(name=name, use=[], decl=decl, contains=contains)

    def contains_block2list(self, ctx:fortranParser.Contains_blockContext):
        # Returns a list
        units = []
        for unit in ctx.sub_or_func():
            units.append(self.visit(unit))
        return units

    # Visit a parse tree produced by fortranParser#contains_block.
    def visitContains_block(self, ctx:fortranParser.Contains_blockContext):
        pass

    # Visit a parse tree produced by fortranParser#private_decl.
    def visitPrivate_decl(self, ctx:fortranParser.Private_declContext):
        syms = []
        if ctx.id_list():
            for sym in ctx.id_list().ident():
                syms.append(sym.getText())
        return ast.Private(vars=syms, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#public_decl.
    def visitPublic_decl(self, ctx:fortranParser.Public_declContext):
        syms = []
        if ctx.id_list():
            for sym in ctx.id_list().ident():
                syms.append(sym.getText())
        return ast.Public(vars=syms, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#subroutine.
    def visitSubroutine(self, ctx:fortranParser.SubroutineContext):
        name = ctx.ident(0).getText()
        args = []
        if ctx.id_list():
            for arg in ctx.id_list().ident():
                args.append(ast.arg(arg=arg.getText()))
        use, decl, body, contains = self.process_sub_block(ctx.sub_block())
        return ast.Subroutine(name=name, args=args, use=use,
                decl=decl, body=body, contains=contains,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#function.
    def visitFunction(self, ctx:fortranParser.FunctionContext):
        name = ctx.ident(0).getText()
        args = []
        if ctx.id_list():
            for arg in ctx.id_list().ident():
                args.append(ast.arg(arg=arg.getText()))
        use, decl, body, contains = self.process_sub_block(ctx.sub_block())
        return_type = None
        if ctx.ident(1):
            return_var = ast.Name(id=ctx.ident(1).getText())
        else:
            return_var = None
        return ast.Function(name=name, args=args, return_type=return_type,
                return_var=return_var, bind=None, use=use, decl=decl, body=body,
                contains=contains,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#subroutine_call.
    def visitSubroutine_call(self, ctx:fortranParser.Subroutine_callContext):
        name = ctx.ident().getText()
        args = []
        if ctx.arg_list():
            for arg in ctx.arg_list().arg():
                if arg.ident():
                    # TODO: handle kwargs, we ignore the name for now
                    kwarg = arg.ident().getText()
                args.append(self.visit(arg))
        return ast.SubroutineCall(name, args, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#builtin_statement.
    def visitBuiltin_statement(self, ctx:fortranParser.Builtin_statementContext):
        name = ctx.name.text
        args = []
        if ctx.arg_list():
            for arg in ctx.arg_list().arg():
                if arg.ident():
                    # TODO: handle kwargs, we ignore the name for now
                    kwarg = arg.ident().getText()
                args.append(self.visit(arg))
        return ast.BuiltinCall(name, args, lineno=1, col_offset=1)


    # Visit a parse tree produced by fortranParser#assignment_statement.
    def visitAssignment_statement(self, ctx:fortranParser.Assignment_statementContext):
        if ctx.op.text == "=>":
            raise Exception("operator => not implemented")
        assert ctx.op.text == "="
        lhs = self.visit(ctx.expr(0))
        value = self.visit(ctx.expr(1))
        return ast.Assignment(lhs, value, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#var_decl.
    def visitVar_decl(self, ctx:fortranParser.Var_declContext):
        attrs = []
        for a in ctx.var_modifier():
            if a.KW_INTENT():
                if a.KW_IN():
                    arg = ast.attribute_arg(arg="in")
                elif a.KW_OUT():
                    arg = ast.attribute_arg(arg="out")
                elif a.KW_INOUT():
                    arg = ast.attribute_arg(arg="inout")
                else:
                    assert False
                attr = ast.Attribute(name=a.KW_INTENT().getText().lower(),
                    args=[arg])
            else:
                attr = ast.Attribute(name=a.getText().lower(), args=[])
            attrs.append(attr)
        d = []
        for v in ctx.var_sym_decl():
            sym = v.ident().getText()
            sym_type = ctx.var_type().getText().lower()
            dims = []
            if v.array_decl():
                for dim in v.array_decl().array_comp_decl():
                    e = dim.expr()
                    if len(e) == 1:
                        end = self.visit(e[0])
                    else:
                        # TODO: implement expr:expr
                        end = None
                    dims.append(ast.dimension(start=None, end=end))
            d.append(ast.decl(sym, sym_type, dims, attrs))
        return ast.Declaration(d, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#print_statement.
    def visitPrint_statement(self, ctx:fortranParser.Print_statementContext):
        values = []
        for expr in ctx.expr_list().expr():
            v = self.visit(expr)
            values.append(v)
        return ast.Print(fmt=None, values=values, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_array_const.
    def visitExpr_array_const(self, ctx:fortranParser.Expr_array_constContext):
        values = []
        for expr in ctx.expr_list().expr():
            v = self.visit(expr)
            values.append(v)
        return ast.ArrayInitializer(args=values, lineno=1, col_offset=1)

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
        v = ctx.ident().getText()
        return ast.Name(id=v, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_not.
    def visitExpr_not(self, ctx:fortranParser.Expr_notContext):
        rhs = self.visit(ctx.expr())
        return ast.UnaryOp(op=ast.Not(), operand=rhs,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_and.
    def visitExpr_and(self, ctx:fortranParser.Expr_andContext):
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        return ast.BoolOp(left=lhs, op=ast.And(), right=rhs,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_or.
    def visitExpr_or(self, ctx:fortranParser.Expr_orContext):
        lhs = self.visit(ctx.expr(0))
        rhs = self.visit(ctx.expr(1))
        return ast.BoolOp(left=lhs, op=ast.Or(), right=rhs,
                lineno=1, col_offset=1)

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
            ".eq.": ast.Eq(),
            "/=": ast.NotEq(),
            ".neq.": ast.NotEq(),
            "<": ast.Lt(),
            ".lt.": ast.Lt(),
            "<=": ast.LtE(),
            ".le.": ast.LtE(),
            ">": ast.Gt(),
            ".gt.": ast.Gt(),
            ">=": ast.GtE(),
            ".ge.": ast.GtE(),
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

    # Visit a parse tree produced by fortranParser#if_block.
    def visitIf_block(self, ctx:fortranParser.If_blockContext):
        cond = self.visit(ctx.if_cond().expr())
        body = self.statements2list(ctx.statements())
        if ctx.if_else_block():
            orelse = self.visit(ctx.if_else_block())
        else:
            orelse = []
        return ast.If(test=cond, body=body, orelse=orelse,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#if_else_block.
    def visitIf_else_block(self, ctx:fortranParser.If_else_blockContext):
        # This method returns a list [] of statements.
        if ctx.statements():
            return self.statements2list(ctx.statements())
        if ctx.if_block():
            return [self.visit(ctx.if_block())]
        raise Exception("The grammar should not allow this.")

    # Visit a parse tree produced by fortranParser#where_single_line.
    def visitWhere_single_line(self, ctx:fortranParser.Where_single_lineContext):
        cond = self.visit(ctx.where_cond().expr())
        body = self.visit(ctx.statement())
        if not isinstance(body, list):
            body = [body]
        return ast.Where(test=cond, body=body, orelse=[],
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#where_block.
    def visitWhere_block(self, ctx:fortranParser.Where_blockContext):
        cond = self.visit(ctx.where_cond().expr())
        body = self.statements2list(ctx.statements())
        if ctx.where_else_block():
            orelse = self.visit(ctx.where_else_block())
        else:
            orelse = []
        return ast.Where(test=cond, body=body, orelse=orelse,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#where_else_block.
    def visitWhere_else_block(self, ctx:fortranParser.Where_else_blockContext):
        # This method returns a list [] of statements.
        if ctx.statements():
            return self.statements2list(ctx.statements())
        if ctx.where_block():
            return [self.visit(ctx.where_block())]
        raise Exception("The grammar should not allow this.")

    # Visit a parse tree produced by fortranParser#expr_array_call.
    def visitExpr_array_call(self, ctx:fortranParser.Expr_array_callContext):
        name = ctx.ident().getText()
        args = []
        for arg in ctx.array_index_list().array_index():
            args.append(self.visit(arg))
        return ast.Array(name, args, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#expr_fn_call.
    def visitExpr_fn_call(self, ctx:fortranParser.Expr_fn_callContext):
        name = ctx.ident().getText()
        args = []
        if ctx.arg_list():
            for arg in ctx.arg_list().arg():
                if arg.ident():
                    # TODO: handle kwargs, we ignore the name for now
                    kwarg = arg.ident().getText()
                args.append(self.visit(arg))
        return ast.FuncCallOrArray(func=name, args=args, keywords=[],
                    lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#array_index_simple.
    def visitArray_index_simple(self, ctx:fortranParser.Array_index_simpleContext):
        return ast.ArrayIndex(left=self.visit(ctx.expr()), right=None,
                step=None)

    # Visit a parse tree produced by fortranParser#array_index_slice.
    def visitArray_index_slice(self, ctx:fortranParser.Array_index_sliceContext):
        # TODO: figure out how to access things like 3:4, or :4.
        return ast.ArrayIndex(left=None, right=None,
                step=None)

    # Visit a parse tree produced by fortranParser#do_statement.
    def visitDo_statement(self, ctx:fortranParser.Do_statementContext):
        if ctx.ident():
            var = ctx.ident().getText()
            start = self.visit(ctx.expr(0))
            end = self.visit(ctx.expr(1))
            if len(ctx.expr()) == 3:
                increment = self.visit(ctx.expr(2))
            else:
                increment = None
            head = ast.do_loop_head(var=var, start=start, end=end,
                    increment=increment)
        else:
            head = None
        body = self.statements2list(ctx.statements())
        return ast.DoLoop(head=head, body=body, lineno=1, col_offset=1)

    def statements2list(self, ctx:fortranParser.StatementsContext):
        # Returns a list
        statements = []
        for stat in ctx.statement():
            s = self.visit(stat)
            if s is not None:
                statements.append(s)
        return statements

    # Visit a parse tree produced by fortranParser#while_statement.
    def visitWhile_statement(self, ctx:fortranParser.While_statementContext):
        test = self.visit(ctx.expr())
        body = self.statements2list(ctx.statements())
        return ast.WhileLoop(test=test, body=body, lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#cycle_statement.
    def visitCycle_statement(self, ctx:fortranParser.Cycle_statementContext):
        return ast.Cycle(lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#exit_statement.
    def visitExit_statement(self, ctx:fortranParser.Exit_statementContext):
        return ast.Exit(lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#select_statement.
    def visitSelect_statement(self, ctx:fortranParser.Select_statementContext):
        expr = self.visit(ctx.expr())
        body = []
        for case in ctx.case_statement():
            test = self.visit(case.expr())
            case_body = self.statements2list(case.statements())
            body.append(ast.case_stmt(test=test, body=case_body))
        if ctx.select_default_statement():
            default_body = self.statements2list( \
                    ctx.select_default_statement().statements())
            default = ast.case_default(body=default_body)
        else:
            default = None
        return ast.Select(test=expr, body=body, default=default,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#use_statement.
    def visitUse_statement(self, ctx:fortranParser.Use_statementContext):
        module = self.visit(ctx.use_symbol())
        symbols = []
        if ctx.use_symbol_list():
            for s in ctx.use_symbol_list().use_symbol():
                symbols.append(self.visit(s))
        return ast.Use(module=module, symbols=symbols,
                lineno=1, col_offset=1)

    # Visit a parse tree produced by fortranParser#use_symbol.
    def visitUse_symbol(self, ctx:fortranParser.Use_symbolContext):
        if len(ctx.ident()) == 1:
            return ast.use_symbol(sym=ctx.ident(0).getText(), rename=None)
        elif len(ctx.ident()) == 2:
            return ast.use_symbol(sym=ctx.ident(0).getText(),
                    rename=ctx.ident(1).getText())
        else:
            raise Exception("The grammar should not allow this.")

    # Visit a parse tree produced by fortranParser#interface_decl.
    def visitInterface_decl(self, ctx:fortranParser.Interface_declContext):
        name = ctx.ident(0).getText()
        procs = []
        for proc_line in ctx.id_list():
            for proc in proc_line.ident():
                procs.append(proc.getText())
        return ast.Interface(name=name, procs=procs,
                lineno=1, col_offset=1)


def antlr_parse(source, translation_unit=False):
    """
    Parse the `source` string into an AST node.

    We first call ANTLR4 to convert the `source` string into a parse tree. Then
    we convert the parse tree into an AST.

    translation_unit ... if True, only accept full programs or modules (the
    'root' rule). If False, accept any valid Fortran code (the 'unit' rule).
    """
    if source.strip() == "":
        return
    stream = antlr4.InputStream(source)
    lexer = fortranLexer(stream)
    tokens = antlr4.CommonTokenStream(lexer)
    parser = fortranParser(tokens)
    parser.removeErrorListeners()
    err = VerboseErrorListener()
    err.source = source
    parser.addErrorListener(err)
    if translation_unit:
        parse_tree = parser.root()
    else:
        parse_tree = parser.units()
    v = ASTBuilderVisitor()
    ast_tree = v.visit(parse_tree)
    if isinstance(ast_tree, list):
        assert len(ast_tree) > 1
        for x in ast_tree:
            assert isinstance(x, ast.AST)
    elif ast_tree is None:
        pass
    else:
        assert isinstance(ast_tree, ast.AST)
    return ast_tree
